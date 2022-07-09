//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#include "Util.h"

bool HExprSerializer::VisitStmt(Stmt *s) {
  if (s == NULL)
    return true;
  switch (s->getStmtClass()) {
    case clang::Stmt::BinaryOperatorClass:
    case clang::Stmt::CompoundAssignOperatorClass:
    case clang::Stmt::UnaryOperatorClass:
    case clang::Stmt::DeclRefExprClass:
    case clang::Stmt::IntegerLiteralClass:
    case clang::Stmt::FloatingLiteralClass:
    case clang::Stmt::ImaginaryLiteralClass:
    case clang::Stmt::ParenExprClass:
    case clang::Stmt::ArraySubscriptExprClass:
    case clang::Stmt::MemberExprClass:
      output.push_back(cast<Expr>(s));
      break;
    default:
      break;
  }

  return true;
}
