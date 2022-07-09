//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#ifndef UTIL_H
#define UTIL_H

#include "ClangHeader.h"

class HExprSerializer: public RecursiveASTVisitor<HExprSerializer> {
public:
  void clear() {output.clear();}
  std::vector<Expr *> getOutput() {return output;}

  bool VisitStmt(Stmt *s);

private:
  std::vector<Expr *> output;
};

#endif
