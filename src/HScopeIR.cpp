//------------------------------------------------------------------------------


#include <iostream>
#include "HScopeIR.h"
#include "HLoopInfo.h"
#include "HFunctionInfo.h"
#include "Util.h"

// Class HPosition
HPosition::HPosition(SourceLocation loc) {
  line = SM->getExpansionLineNumber(loc);
  col = SM->getExpansionColumnNumber(loc);
}

HPosition HPosition::operator=(const HPosition & pos){
  line = pos.line;
  col = pos.col;
  return pos;
}

bool HPosition::operator<(const HPosition & pos){
  return (line < pos.line || (line == pos.line && col < pos.col));
}

bool HPosition::operator>(const HPosition & pos){
  return (line > pos.line || (line == pos.line && col > pos.col));
}

bool HPosition::operator>=(const HPosition & pos){
  return (line >= pos.line || (line == pos.line && col >= pos.col));
}

bool HPosition::operator<=(const HPosition & pos){
  return (line <= pos.line || (line == pos.line && col <= pos.col));
}

bool HPosition::operator==(const HPosition & pos){
  return (line == pos.line && col == pos.col);
}


  //Handle DelcRef in index expr.
int evaluateDeclRef(DeclRefExpr *ref, std::string &min, std::string &max) {
  int ret = -1;

  HPosition pos(ref->getLocation());
  HScopeIR * scope = pos_to_scope(pos);

  ValueDecl * d = ref->getDecl();

  if (d->getKind() == clang::Decl::Var) {
    HDecl_Var *dv = scope->find_var(d);
    if (dv) {
      HAccess_Var *av = dv->find_last_access(pos, true);
      if (av) {
        HValueRange * v = av->get_value();
        if (v) {
          ret = v->get_type();
          min = v->get_lower_str();
          max = v->get_upper_str();
          return ret;
        }
      }
    }
  }
  min = d->getName().str();
  max = min;
  return ret;
}
//Handle ParenExpr in BinaryOperator.
int evaluateParen (ParenExpr *pe, std::string &min, std::string &max) {
  int ret = -1;
  std::string sub_min;
  std::string sub_max;
  Expr *e = pe->getSubExpr()->IgnoreImpCasts();
  ret = evaluate (e, sub_min, sub_max);

  if (ret == -1) {
    min = "(" + sub_min + ")";
    max = "(" + sub_max + ")";
  }
  else {
    min = sub_min;
    max = sub_max;
  }
  return ret;
}
//Analysis binary operator, give its min value and max value.
int evaluateBinop(BinaryOperator *op, std::string &min, std::string &max) {

  int ret = 0;
  int l_ret = 0;
  int r_ret = 0;
  std::string lmin, lmax, rmin, rmax;

  Expr *lhs = op->getLHS()->IgnoreImpCasts();
  l_ret = evaluate (lhs, lmin, lmax);

  std::string op_str = op->getOpcodeStr().str();

  Expr *rhs = op->getRHS()->IgnoreImpCasts();
  r_ret = evaluate (rhs, rmin, rmax);

  ret = 1;
  int lii, lia, rii, ria;
  float lfi, lfa, rfi, rfa;
  if (l_ret == -1 || r_ret == -1) {
    ret = -1;
    min = lmin + op_str + rmin;
    max = lmax + op_str + rmax;
  } else if (l_ret == 0 && r_ret == 0) {
    ret = 0;
    lii = std::atoi(lmin.c_str());
    lia = std::atoi(lmax.c_str());
    rii = std::atoi(rmin.c_str());
    ria = std::atoi(rmax.c_str());
  } else {
    lfi = std::atof(lmin.c_str());
    lfa = std::atof(lmax.c_str());
    rfi = std::atof(rmin.c_str());
    rfa = std::atof(rmax.c_str());
  }

  switch (op->getOpcode ()) {
    case BO_Mul:
      if (ret == 0) {
  	min = std::to_string((lii * rii));
  	max = std::to_string((lia * ria));
      }
      else if (ret == 1) {
  	min = std::to_string((lfi * rfi));
  	max = std::to_string((lfa * rfa));
      }
      break;
    case BO_Add:
      if (ret == 0) {
  	min = std::to_string((lii + rii));
  	max = std::to_string((lia + ria));
      }
      else if (ret == 1) {
  	min = std::to_string((lfi + rfi));
  	max = std::to_string((lfa + rfa));
      }
      break;
    case BO_Sub:
      if (ret == 0) {
  	min = std::to_string((lii - ria));
  	max = std::to_string((lia - rii));
      }
      else if (ret == 1) {
  	min = std::to_string((lfi - rfa));
  	max = std::to_string((lfa - rfi));
      }
      else {
        min = lmin + op_str + rmax;
        max = lmax + op_str + rmin;
      }
      break;
    case BO_Div:
      if (ret == 0) {
  	min = std::to_string((lii / ria));
  	max = std::to_string((lia / rii));
      }
      else if (ret == 1) {
  	min = std::to_string((lfi / rfa));
  	max = std::to_string((lfa / rfi));
      }
      else {
        min = lmin + op_str + rmax;
        max = lmax + op_str + rmin;
      }
      break;
    case BO_AddAssign:
    case BO_SubAssign:
    case BO_MulAssign:
    case BO_DivAssign:
    case BO_AndAssign:
    case BO_XorAssign:
    case BO_OrAssign:
    case BO_Assign:
    case BO_Rem:
    case BO_Shl:
    case BO_Shr:
    case BO_EQ:
    case BO_NE:
    case BO_LE:
    case BO_GE:
    case BO_LT:
    case BO_GT:
    case BO_And:
    case BO_Xor:
    case BO_Or:
    case BO_LAnd:
    case BO_LOr:
    default:
      min = lmin + op_str + rmax;
      max = lmax + op_str + rmin;
      ret = -1;
      break;
  }
  return ret;
}

int evaluateUnop(UnaryOperator *up, std::string &min, std::string &max) {
  int ret = -1;
  std::string sub_min, sub_max;
  int ii, ia;
  float fi, fa;
  
  Expr * e = up->getSubExpr();
  std::string up_str = up->getOpcodeStr(up->getOpcode()).str();

  ret = evaluate (e, sub_min, sub_max);
  if (ret == 0) {
    ii = std::atoi(sub_min.c_str());
    ia = std::atoi(sub_max.c_str());
  }
  else if (ret == 1) {
    fi = std::atof(sub_min.c_str());
    fa = std::atof(sub_max.c_str());
  }

  switch (up->getOpcode ()) {
    case UO_PostInc:
    case UO_PostDec:
      if (ret == 1) {
        min = sub_min + up_str;
        max = sub_max + up_str;
      }
      else {
  	min = sub_min;
  	max = sub_max;
      }
      break;
    case UO_PreInc:
      if (ret == 0) {
  	min = std::to_string((ii + 1));
  	max = std::to_string((ia + 1));
      }
      else if (ret == 1) {
  	min = std::to_string((fi + 1));
  	max = std::to_string((fa + 1));
      }
      else {
        min = up_str + sub_max;
        max = up_str + sub_min;
      }
      break;
    case UO_PreDec:
      if (ret == 0) {
  	min = std::to_string((ii - 1));
  	max = std::to_string((ia - 1));
      }
      else if (ret == 1) {
  	min = std::to_string((fi - 1));
  	max = std::to_string((fa - 1));
      }
      else {
        min = up_str + sub_max;
        max = up_str + sub_min;
      }
      break;
    case UO_Minus:
      if (ret == 0) {
  	min = std::to_string((ii * -1));
  	max = std::to_string((ia * -1));
      }
      else if (ret == 1) {
  	min = std::to_string((fi * -1));
  	max = std::to_string((fa * -1));
      }
      else {
        min = up_str + sub_max;
        max = up_str + sub_min;
      }
      break;
    case UO_Not:
    case UO_LNot:
    default:
      min = up_str + sub_max;
      max = up_str + sub_min;
      ret = -1;
      break;
  }

  return ret;
}
//Evaluate the algebra expression in this context.
//Return -1: Expr; 0: integer; 1: float;
int evaluate(Expr * e, std::string &min_str, std::string &max_str) {
  int ret = -1;
  DeclRefExpr *ref ;
  int m_num=SM->getExpansionLineNumber(e->getExprLoc());
  int col_num=SM->getExpansionColumnNumber(e->getExprLoc());

  switch (e->getStmtClass()) {
    case Stmt::BinaryOperatorClass:
      ret = evaluateBinop (cast<BinaryOperator> (e), min_str, max_str);
      break;
    case Stmt::DeclRefExprClass:
    {
      ref = cast<DeclRefExpr>(e);
      ret = evaluateDeclRef (ref, min_str, max_str);
      break;
    }
    case Stmt::IntegerLiteralClass:{
      IntegerLiteral * int_lit = cast<IntegerLiteral>(e);
      llvm::APInt int_val = int_lit->getValue();
      int * p_val = (int *)int_val.getRawData();
      ret = 0;
      min_str = std::to_string (*p_val);
      max_str = std::to_string (*p_val);
      break;
    }

    case Stmt::ParenExprClass:
    {
      ret = evaluateParen((ParenExpr*) e, min_str, max_str);
      break;
      }
    case Stmt::UnaryOperatorClass:
      ret = evaluateUnop (cast<UnaryOperator> (e), min_str, max_str);
      break;
    case Stmt::ImaginaryLiteralClass:
    case Stmt::ArraySubscriptExprClass:
    case Stmt::MemberExprClass:
    case Stmt::FloatingLiteralClass:
    default:
      std::cerr <<"[Warning]Unrecognized Expression!\n";
      e->dump();
      ret = -1;
      min_str = "Unrecognized";
      max_str = "Unrecognized";
      break;
  }

  return ret;
}
// Class HValueRange Implementation.
HValueRange::HValueRange() {
  type = -2;
  value = NULL;
  lower_boundary_str.clear();
  upper_boundary_str.clear();
}

HValueRange::~HValueRange() {
}

HValueRange::HValueRange(Expr *e) {
  value = e;
  type = evaluate(e, lower_boundary_str, upper_boundary_str);
}

void HValueRange::set_value (Expr *e) {
  value = e;
  type = evaluate(e, lower_boundary_str, upper_boundary_str);
}

// Class HDecl_Var Implementation.
// For each variable declar.
HDecl_Var::HDecl_Var() {
  init_value = NULL;
  pos = NULL;
  scope = NULL;
}

HDecl_Var::HDecl_Var(ValueDecl * d) {
  init_value = NULL;
  pos = new HPosition(d->getLocation());
  scope = CurScope;
  name = d->getName().str();
  decl_stmt = d;
  type = d->getType().getAsString();

  switch (d->getKind()) {
    case Decl::ParmVar:
	category = 1;
	break;
    case Decl::Function:
	category = 0;
	break;
    case Decl::Var:
	category = 3;
	break;
    default:
	break;
  }

  //TODO: How to handle array defines like this "int a[4][5]" ?
  if (category > 0 && type.find("*") != std::string::npos)
    category ++;

}

HDecl_Var::~HDecl_Var() {
    if (init_value != NULL)
      delete init_value;
    if (pos != NULL)
      delete pos;
}

//TODO: We have not consider the loop context;
HAccess_Var * HDecl_Var::find_last_access(HPosition &pos, bool iswrite){
  for (int i = (int)access_chain.size() - 1; i >= 0; i--) {
    if ((access_chain[i]->pos->get_line()) < pos.get_line() && access_chain[i]->isWrite() == iswrite)
      return access_chain[i];
  }

  return NULL;
}

HAccess_Var * HDecl_Var::find_next_access(HPosition &pos, bool iswrite){
  for (auto& av : access_chain) {
    if (*(av->pos) > pos && av->isWrite() == iswrite)
      return av;
  }

  return NULL;
}

int HDecl_Var::access_type(HScopeIR *s) {
  int ret = 0;
  for (auto& av: access_chain) {
    if (s->isInside(av->get_scope())) {
      if (av->isWrite())
	ret |= 2;
      else
	ret |= 1;
      if (ret == 3)
	return ret;
    }
  }

  return ret;
}

HDecl_Mem::~HDecl_Mem() {
    if (elem_num != NULL)
      delete elem_num;
    if (total_size != NULL)
      delete total_size;
}

// Class HAccess_Var Implementation.
// For each access of one variable.
HAccess_Var::HAccess_Var(DeclRefExpr * ref) {
  value_range = NULL;
  pos = new HPosition(ref->getLocation());
  scope = CurScope;
  decl_var = scope->find_var(ref->getDecl());
}

HAccess_Var::HAccess_Var(VarDecl * d) {
  value_range = NULL;
  pos = new HPosition(d->getLocation());
  scope = CurScope;
  decl_var = scope->find_var(d);
}

HAccess_Var::HAccess_Var() {
  decl_var = NULL;
  value_range = NULL;
}

HAccess_Var::~HAccess_Var() {
  if (value_range != NULL)
    delete value_range;
}

HAccess_Mem::HAccess_Mem() {
    decl_var = NULL;
    value_range = NULL;
}

HAccess_Mem::~HAccess_Mem() {
    if (value_range != NULL)
        delete value_range;
}

HAccess_Var * HAccess_Var::find_last_access(ValueDecl * decl_stmt, bool iswrite) {
  HDecl_Var * decl_var = scope->find_var(decl_stmt);
  if (decl_var == NULL)
    return NULL;

  return decl_var->find_last_access(*pos, iswrite);
}

HAccess_Var * HAccess_Var::find_next_access(ValueDecl * decl_stmt, bool iswrite) {
  HDecl_Var * decl_var = scope->find_var(decl_stmt);
  if (decl_var == NULL)
    return NULL;

  return decl_var->find_next_access(*pos, iswrite);
}

void HAccess_Var::set_value (Expr *e) {
  value_range = new HValueRange(e);
}

void HAccess_Var::set_value_max (Expr *e) {
   std::string min_nothing;
   int t = evaluate(e,min_nothing,value_range->upper_boundary_str);
}

void HAccess_Var::set_value_min (Expr *e) {
    std::string max_nothing;
    int t = evaluate(e,value_range->lower_boundary_str,max_nothing);
}

HAccess_Mem::HAccess_Mem(DeclRefExpr * ref) {
    value_range = NULL;
    pos = new HPosition(ref->getLocation());
    scope = CurScope;
    decl_var = scope->find_var(ref->getDecl());
    is_mem=false;
}

HAccess_Mem::HAccess_Mem(VarDecl * d) {
    value_range = NULL;
    pos = new HPosition(d->getLocation());
    scope = CurScope;
    decl_var = scope->find_var(d);
    is_mem=false;
}

HAccess_Mem::HAccess_Mem(ArraySubscriptExpr * ArrEx) {
  value_range = NULL;
  Expr *base = ArrEx->getBase()->IgnoreImpCasts();
  set_index(ArrEx->getIdx()->IgnoreImpCasts());
  decl_var = NULL;
  scope = CurScope;
  is_mem = true;
  for(auto &acc_av_display:scope->access_chain){
      int i=0;
  }

  while (isa<ArraySubscriptExpr>(base)) {
    ArraySubscriptExpr * a = dyn_cast<ArraySubscriptExpr>(base);
    set_index(a->getIdx()->IgnoreImpCasts());
    base = a->getBase()->IgnoreImpCasts();
  }

  if (DeclRefExpr * ref = dyn_cast<DeclRefExpr>(base)) {
    pos = new HPosition(ref->getLocation());
    if(scope->type==0){
        int idx=0;
        for(auto child:scope->children){
            if(pos->get_line()<child->get_line()){
                scope=scope->children[idx-1];
                break;
            }
            idx++;
        }
        if (scope->get_type()==0)
            scope=scope->children[idx-1];
    }
    decl_var = scope->find_var(ref->getDecl());
  }
  else {
    ArrEx->dump();
    std::cerr <<"Exception Array\n";
  }
}

void HAccess_Mem::set_index (Expr *e) {
  index.push_back(e);
  /*
  if(isa<DeclRefExpr>(e)){
      DeclRefExpr *ref = dyn_cast<DeclRefExpr>(e);
      HPosition pos(ref->getLocation());
      HScopeIR * scope_internal = pos_to_scope(pos);
      ValueDecl * d = ref->getDecl();
      while(scope_internal->type==2){
          for(auto av_acc_internal:scope_internal->access_chain){
              if(av_acc_internal->get_decl()->get_name()==d->getNameAsString())
          }

      }
  }
  */
  HValueRange * v = new HValueRange(e);
  index_value.push_back(v);
}
/*
HAccess_Var::~HAccess_Var() {
  index.clear();
  for (auto& v : index_value)
    delete v;
  index_value.clear();
}
*/
// Class HScopeIR implementation.
HScopeIR::HScopeIR() {
  parent = NULL;
  scope_st = NULL;
  //function = NULL;
  type = -2;
  insns = 0;
}

HScopeIR::~HScopeIR() {

  for (auto& child : children)
    delete child;

  for (auto& dv : decl_chain)
    delete dv;

  for (auto& av : access_chain)
    delete av;

}

void HScopeIR::dump() {
  if (scope_st)
    scope_st->dump();
}

HDecl_Mem * HScopeIR::find_var(ValueDecl * decl_stmt){
    /*
  if((this->type==2||this->type==3||this->type==4)&&this->decl_chain.empty())  {
      HScopeIR *scope_info=pos_to_scope(*this->pos);
      for (auto& dv : scope_info->decl_chain) {
          if (dv->decl_stmt == decl_stmt)
              return dv;
      }
  }
  else{
      for (auto& dv : decl_chain) {
          if (dv->decl_stmt == decl_stmt)
              return dv;
      }
  }
     */
  for (auto& dv : decl_chain) {
      if (dv->decl_stmt == decl_stmt)
          return dv;
  }
  if (parent == NULL)
    return NULL;

  return parent->find_var(decl_stmt);
}
bool  HScopeIR::find_var(HAccess_Mem * acc_var){
    for (auto& dv : access_chain) {
        if (dv == acc_var)
            return true;
    }
    return false;
}
void HScopeIR::add_child(HScopeIR * scope) {
  if (scope == NULL)
    return;
  scope->parent = this;
  children.push_back(scope);
}

void HScopeIR::addUDChain(HAccess_Mem * acc_var) {
  if (acc_var == NULL)
    return;

  if (acc_var->isValid()){

    if(access_chain.empty())
        this->access_chain.push_back(acc_var);
    if(find_var(acc_var)==NULL)
        this->access_chain.push_back(acc_var);

//    this->access_chain.push_back(acc_var);
    HDecl_Var * var = acc_var->get_decl();
    var->append_access(acc_var);

  }
}

unsigned int HScopeIR::get_call_num() {
  unsigned int sum = callers.size();
  for (auto& child : children) 
    sum += child->get_call_num();

  return sum;
}

unsigned int HScopeIR::get_insns_num() {
  unsigned int sum = insns;
  for (auto& child : children) 
    sum += child->get_insns_num();

  return sum;
}

bool HScopeIR::isInside(HScopeIR *s) {
  if (this == s)
    return true;

  for (auto&child : children)
    if (child->isInside(s) == true)
      return true;
  
  return false;
}

HLoopInfo * HScopeIR::get_loop() {
  if (type <= 4 && type >= 2)
  {
//      HLoopInfo *p_loop_fornow = new HLoopInfo();
//      p_loop_fornow->decl_chain=this->decl_chain;
      return  (HFunctionInfo*)this;
  }
  if (type == 1)
      return (HFunctionInfo*)this;

  return parent->get_func();
}

HFunctionInfo  * HScopeIR::get_func() {
  if (type == 1)
    return  (HFunctionInfo*)this;
  if(parent!=NULL)
      return parent->get_func();

}

//Find if the Algebra expression is related to Var, it will trace the compute chain.
bool isVarRelatedExpr(Expr * e, ValueDecl * v) {
  HExprSerializer se; 
  se.clear();
  se.TraverseStmt(e);
  std::vector<Expr *> se_e = se.getOutput();
  HPosition pos(e->getLocStart());
  HPosition pos_e(e->getLocEnd());
  HScopeIR * s = pos_to_scope(pos);
  std::vector<ValueDecl *> se_v;
  for (unsigned int i = 0; i < se_e.size(); i++) {
    if(DeclRefExpr * dr = dyn_cast<DeclRefExpr> (se_e[i])) {
      ValueDecl * d = dr->getDecl();
      if (d == v)
	return true;
      else if (d->getKind() == clang::Decl::Var) {
	HDecl_Var *dv = s->find_var(d);
	HAccess_Var *av = dv->find_last_access(pos, true);
	if (av && isVarRelatedExpr(av->get_value()->get_value(), v))
	  return true;
      }
    }
  }

  return false;
}

//FIXME: This function need to be implement.
bool isVarAffineRelatedExpr(Expr * e, ValueDecl * v) {

  return isVarRelatedExpr(e, v);
}

//Compare two algebra expression has the same value in this context.
//TODO:We do not trace the compute chain right now.
//TODO: Compare two expression, return -2 means can not compare; -1 means e1 > e2; 0 means e1 == e2; 1 means e1 < e2.
//TODO: We can rewrite the compute chain, reorder the operators and operation by lexicographical order.
int compareAlgebraExpr(Expr * e1, Expr * e2) {
  int ret = -2;
  HExprSerializer se; 

  se.clear();
  se.TraverseStmt(e1);
  std::vector<Expr *> se_e1 = se.getOutput();

  se.clear();
  se.TraverseStmt(e2);
  std::vector<Expr *> se_e2 = se.getOutput();

  if (se_e1.size() != se_e2.size())
    return ret;

  for (unsigned int i = 0; i < se_e1.size(); i++) {
    if (se_e1[i]->getStmtClass() != se_e2[i]->getStmtClass())
      return ret;
    switch (se_e1[i]->getStmtClass()) {
      case Stmt::BinaryOperatorClass:
      case Stmt::CompoundAssignOperatorClass:
	{
	  BinaryOperator * BinOp1 = cast<BinaryOperator>(se_e1[i]);
	  BinaryOperator * BinOp2 = cast<BinaryOperator>(se_e2[i]);
	  if ( BinOp1->getOpcode() != BinOp2->getOpcode())
	    return ret;
	}
	break;
      case Stmt::UnaryOperatorClass:
	{
          UnaryOperator *UnOp1 = dyn_cast<UnaryOperator>(se_e1[i]);
          UnaryOperator *UnOp2 = dyn_cast<UnaryOperator>(se_e2[i]);
	  if (UnOp1->getOpcode() != UnOp2->getOpcode())
	    return ret;
	}
	break;
      case Stmt::DeclRefExprClass:
	{
	  DeclRefExpr * DeRef1 = cast<DeclRefExpr> (se_e1[i]);
	  DeclRefExpr * DeRef2 = cast<DeclRefExpr> (se_e2[i]);
	  if (DeRef1->getDecl() != DeRef2->getDecl()) 
	    return ret;
	}
	break;
      case Stmt::IntegerLiteralClass:
	{
          IntegerLiteral * int1 = cast<IntegerLiteral>(se_e1[i]);
          llvm::APInt llvm_val1 = int1->getValue();

          IntegerLiteral * int2 = cast<IntegerLiteral>(se_e2[i]);
          llvm::APInt llvm_val2 = int2->getValue();
 
	  if (llvm_val1.eq(llvm_val2) == false)
	    return ret;
	}
	break;

      case Stmt::ImaginaryLiteralClass:
      case Stmt::ParenExprClass:
      case Stmt::ArraySubscriptExprClass:
      case Stmt::MemberExprClass:
	break;
      default:
	break;
    }
  }

  ret = 0;

  return ret;
}

HScopeIR * pos_to_scope (HPosition &p) {
  HScopeIR * s;
  if(TopScope->children.size()==1)
      s=TopScope->children.front();
  else
      s = TopScope;
  HPosition * s_p;
  HPosition * e_p;
  bool isChange = true;
  while (isChange) {
    isChange = false;
    for (auto& child_s : s->children) {
      s_p = child_s->get_start_pos();
      e_p = child_s->get_end_pos();
      if ( p.get_line() > (*s_p).get_line() && p.get_line() < (*e_p).get_line()) {
        s = child_s;
	isChange = true;
        break;
      }
      if(p.get_line()==(*s_p).get_line()||p.get_line()==(*e_p).get_line()){
          if((p.get_col()>=(*s_p).get_col())&&(p.get_line()==(*s_p).get_line())){
              s = child_s;
              isChange = true;
              break;
          }
          if((p.get_col()<=(*e_p).get_col())&&(p.get_line()==(*e_p).get_line())){
              s = child_s;
              isChange = true;
              break;
          }
      }
    }
  }

  return s;
}
