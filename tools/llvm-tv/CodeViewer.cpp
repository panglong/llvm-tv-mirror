#include "CodeViewer.h"
#include "TVApplication.h"
#include "TVTreeItem.h"
#include "TVFrame.h"
#include "llvm/Function.h"
#include "llvm/Instruction.h"
#include "llvm/Value.h"
#include "llvm/ADT/StringExtras.h"
#include <wx/listctrl.h>
#include <map>
#include <sstream>
using namespace llvm;

void TVCodeItem::SetLabel() {
  std::string label;
  if (!Val)
    label = "<badref>";
  else if (BasicBlock *BB = dyn_cast<BasicBlock>(Val))
    label = BB->getName().str() + ":";
  else if (Instruction *I = dyn_cast<Instruction>(Val)) {
    std::ostringstream out;
    I->print(out);
    label = out.str();
    // Post-processing for more attractive display
    for (unsigned i = 0; i != label.length(); ++i)
      if (label[i] == '\n') {                            // \n => space
        label[i] = ' ';
      } else if (label[i] == '\t') {                     // \t => 2 spaces
        label[i] = ' ';
        label.insert(label.begin()+i+1, ' ');
      } else if (label[i] == ';') {                      // Delete comments!
        unsigned Idx = label.find('\n', i+1);            // Find end of line
        label.erase(label.begin()+i, label.begin()+Idx);
        --i;
      }

  } else
    label = "<invalid value>";

  SetText(wxString(label.c_str(), wxConvUTF8));
}

//===----------------------------------------------------------------------===//

void TVCodeListCtrl::refreshView() {
  // Hide the list while rewriting it from scratch to speed up rendering
  Hide();

  // Clear out the list and then re-add all the items.
  long index = 0;
  ClearAll();
  for (Items::iterator i = itemList.begin(), e = itemList.end(); i != e; ++i) {
    InsertItem(index, (**i).m_text.c_str());
    ItemToIndex[*i] = index;
    ++index;
  }

  Show();
}

template<class T> void wipe (T x) { delete x; }

void TVCodeListCtrl::SetFunction (Function *F) {
  // Empty out the code list.
  if (!itemList.empty ())
    for_each (itemList.begin (), itemList.end (), wipe<TVCodeItem *>);
  itemList.clear ();
  ValueToItem.clear ();

  // Populate it with BasicBlocks and Instructions from F.
  for (Function::iterator BB = F->begin(), BBe = F->end(); BB != BBe; ++BB) {
    TVCodeItem *TCBB = new TVCodeItem(BB);
    itemList.push_back(TCBB);
    ValueToItem[BB] = TCBB;
    for (BasicBlock::iterator I = BB->begin(), Ie = BB->end(); I != Ie; ++I) {
      TVCodeItem *TCI = new TVCodeItem(I);
      itemList.push_back(TCI);
      ValueToItem[I] = TCI;
    }
  }

  // Since the function changed, we had better make sure that the list control
  // matches what's in the code list.
  refreshView ();
}

TVCodeListCtrl::TVCodeListCtrl(wxWindow *_parent, llvm::Function *F)
  : wxListCtrl(_parent, LLVM_TV_CODEVIEW_LIST, wxDefaultPosition, wxDefaultSize,
               wxLC_LIST) {
  SetFunction (F);
}

TVCodeListCtrl::TVCodeListCtrl(wxWindow *_parent)
  : wxListCtrl(_parent, LLVM_TV_CODEVIEW_LIST, wxDefaultPosition, wxDefaultSize,
               wxLC_LIST) {
}

void TVCodeListCtrl::changeItemTextAttrs (TVCodeItem *item,
                                          const wxColour *newColor,
                                          int newFontWeight) {
  item->m_itemId = ItemToIndex[item];
  item->SetTextColour(*newColor);
  wxFont Font = item->GetFont();
  Font.SetWeight(newFontWeight);
  item->SetFont(Font);
  SetItem(*item);
}

void TVCodeListCtrl::OnItemSelected(wxListEvent &event) {
  Value *V = itemList[event.GetIndex ()]->getValue();
  if (!V) return;

  // Highlight uses in red
  for (User::use_iterator u = V->use_begin(), e = V->use_end(); u != e; ++u)
    changeItemTextAttrs (ValueToItem[*u], wxRED, wxBOLD);

  // Highlight definitions of operands in green
  if (User *U = dyn_cast<User>(V))
    for (User::op_iterator op = U->op_begin(), e = U->op_end(); op != e; ++op)
      if (TVCodeItem *TCI = ValueToItem[*op])
        changeItemTextAttrs (TCI, wxGREEN, wxBOLD);
}

void TVCodeListCtrl::OnItemDeselected(wxListEvent &event) {
  Value *V = itemList[event.GetIndex ()]->getValue();
  if (!V) return;

  // Set uses back to normal
  for (User::use_iterator u = V->use_begin(), e = V->use_end(); u != e; ++u)
    changeItemTextAttrs (ValueToItem[*u], wxBLACK, wxNORMAL);

  // Set definitions of operands back to normal
  if (User *U = dyn_cast<User>(V))
    for (User::op_iterator op = U->op_begin(), e = U->op_end(); op != e; ++op)
      if (TVCodeItem *TCI = ValueToItem[*op])
        changeItemTextAttrs (TCI, wxBLACK, wxNORMAL);

}

BEGIN_EVENT_TABLE (TVCodeListCtrl, wxListCtrl)
  EVT_LIST_ITEM_SELECTED(LLVM_TV_CODEVIEW_LIST,
                         TVCodeListCtrl::OnItemSelected)
  EVT_LIST_ITEM_DESELECTED(LLVM_TV_CODEVIEW_LIST,
                           TVCodeListCtrl::OnItemDeselected)
END_EVENT_TABLE ()

//===----------------------------------------------------------------------===//

void TVCodeViewer::displayItem (TVTreeItemData *item) {
  item->viewCodeOn (this);
}

void TVCodeViewer::viewFunctionCode (Function *F) {
  myListCtrl->SetFunction (F);
}

void TVCodeViewer::viewModuleCode (Module *F) {
  myListCtrl->ClearAll();
}
