//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#ifndef HSCOPEIRGEN_H
#define HSCOPEIRGEN_H

#include "HScopeIR.h"
#include "HLoopInfo.h"
#include "HFunctionInfo.h"




class HScopeIRGen: public RecursiveASTVisitor<HScopeIRGen> {
  private:
  //TODO:If the *ptr is calculated form one base ptr, we may trace it.
  HAccess_Mem * create_av_expr(Expr * e);

  public:

  void Initialize() {
    CurScope = new HScopeIR();
    CurScope->set_type(0);
    TopScope->add_child(CurScope);
  }
  HScopeIRGen();
  ~HScopeIRGen();
  bool VisitFunctionDecl (FunctionDecl *f) ;
  bool VisitVarDecl(VarDecl *d) ;
  bool VisitStmt(Stmt *S) ;
  bool dataTraverseStmtPost (Stmt *st) ;
  bool dataTraverseStmtPre (Stmt *st) ;
  bool TraverseStmt(Stmt *S) ;
};

#endif
