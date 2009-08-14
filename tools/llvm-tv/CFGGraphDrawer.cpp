#include "CFGGraphDrawer.h"
#include "TVTreeItem.h"
#include "llvm/Function.h"
#include "llvm/ModuleProvider.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/PassManager.h"
using namespace llvm;

//===----------------------------------------------------------------------===//

// CFGGraphDrawer implementation

wxImage *CFGGraphDrawer::drawFunctionGraph (Function *fn) {
  ModuleProvider *MP = new ExistingModuleProvider (fn->getParent ());
  FunctionPassManager PM (MP);
  PM.add (createCFGOnlyPrinterPass ());
  PM.run (*fn);
  MP->releaseModule (); // Don't delete it when you go away, says I
  delete MP;
  return buildwxImageFromDotFile ("cfg." + fn->getName().str() + ".dot");
}

std::string CFGGraphDrawer::getDisplayTitle (TVTreeItemData *item) {
  std::string title ("Control-flow graph");
  if (item) title += " of " + item->getTitle ();
  return title;
}
