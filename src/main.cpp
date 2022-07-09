//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
#include "HPassManager.h"
#include <stdlib.h>

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

HScopeIR * TopScope;
SourceManager *SM;
HScopeIR * CurScope;
ASTContext *Ctx;

int main(int argc, const char **argv) {
  if(argc>5){
     std::string order;
     order="ppcg --no-shared-memory --target=opencl ";
     order+=argv[1];
     const char * ordr=order.c_str();
     system(ordr);
  }
  CommonOptionsParser op(argc, argv, ToolingSampleCategory);
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  TopScope = new HScopeIR();
  TopScope->set_type(-1);

  // ClangTool::run accepts a FrontendActionFactory, which is then used to
  // create new objects implementing the FrontendAction interface. Here we use
  // the helper newFrontendActionFactory to create a default factory that will
  // return a new MyFrontendAction object every time.
  // To further customize this, we could create our own factory class.
  int ret = Tool.run(newFrontendActionFactory<MyFrontendAction>().get());

  //delete TopScope;
  return ret;
}
