#ifndef CALLGRAPHDRAWER_H
#define CALLGRAPHDRAWER_H

#include "GraphDrawer.h"
#include "TVSnapshot.h"

namespace llvm {
  class Module;
};

//===----------------------------------------------------------------------===//

// CallGraphDrawer interface

class CallGraphDrawer : public GraphDrawer {
public:
  wxImage *drawModuleGraph (llvm::Module *M);
  CallGraphDrawer (wxWindow *parent) : GraphDrawer (parent) { }
};

#endif // CALLGRAPHDRAWER_H
