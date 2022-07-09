//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#ifndef HPASSMANAGER_H
#define HPASSMANAGER_H

#include "HKernelExtractor.h"
#include "HCodeGen.h"
#include "HScopeIRGen.h"
#include "HFunctionInfo.h"
#include "HLoopInfo.h"
#include "HCodeGen.h"

class HScopeIRGen;
// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
MyASTConsumer(std::vector<struct HKernel_Info> &k_info_queue,
        std::vector<struct HKernel_Info> &k_info_queue_cant) : kernelExtractor(k_info_queue,k_info_queue_cant) {}

  bool HandleTopLevelDecl(DeclGroupRef DR) override;

  virtual void Initialize(ASTContext &Context) {
      Ctx = &Context;
      SM = &(Ctx->getSourceManager());
      scopeGen->Initialize();
    }

private:
    HKernelExtractor kernelExtractor;
    HScopeIRGen *scopeGen;

    void Nouse(HLoopInfo *pInfo);
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
private:
  struct HKernel_Info * select_kernel ();
  int find_arr_name(BinaryOperator *rhs,Array_rw_chain *array_cur);
  int find_incall(CallExpr *call,Array_rw_chain * array_cur);
public:
  MyFrontendAction() {}
//Callback at the end of processing a single input.
  void EndSourceFileAction() override;

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override;

private:
  HCodeGen generator;
  std::vector<struct HKernel_Info> k_info_queue;
  std::vector<struct HKernel_Info> k_info_queue_cant;
};

#endif
