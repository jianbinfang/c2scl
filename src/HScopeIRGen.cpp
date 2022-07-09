

#include "HScopeIRGen.h"
#include <iostream>
#include<string.h>


//TODO:If the *ptr is calculated form one base ptr, we may trace it.
HAccess_Mem * HScopeIRGen::create_av_expr(Expr * e) {
  HAccess_Mem *av = NULL;
  if (DeclRefExpr * ref = dyn_cast<DeclRefExpr>(e)) {
    av = new HAccess_Mem(ref);
  }
  else if (ArraySubscriptExpr * ArrEx = dyn_cast<ArraySubscriptExpr>(e)) {
    av = new HAccess_Mem(ArrEx);
    Expr *base = ArrEx->getBase()->IgnoreImpCasts();
    TraverseStmt(ArrEx->getIdx()->IgnoreImpCasts());
    while (isa<ArraySubscriptExpr>(base)) {
      ArraySubscriptExpr * a = dyn_cast<ArraySubscriptExpr>(base);
      TraverseStmt(a->getIdx()->IgnoreImpCasts());
      base = a->getBase()->IgnoreImpCasts();
    }
  }
  else if (UnaryOperator *UnOp = dyn_cast<UnaryOperator>(e)){
    if (UnOp->getOpcode() == UO_Deref){
      Expr * sub_e = UnOp->getSubExpr()->IgnoreImpCasts();
      if (DeclRefExpr * ref = dyn_cast<DeclRefExpr>(sub_e)) {
        av = new HAccess_Mem(ref);
      }
      else {
        UnOp->dump();
        std::cerr <<"Exception Deref Mem to create HAccess_Var!\n";
      }
    }
  }
  else if (isa<IntegerLiteral>(e))
    return NULL;

  if (av == NULL) {
    e->dump();
    std::cerr <<"Exception Expr to create HAccess_Var!\n";
  }

  return av;
}


  bool HScopeIRGen::VisitFunctionDecl (FunctionDecl *f) {
    if (f->doesThisDeclarationHaveABody()) {
      HDecl_Mem * f_var = new HDecl_Mem(f);
      CurScope->append_decl(f_var);

      HFunctionInfo * p_scope = new HFunctionInfo();
      p_scope->type = 1;
      p_scope->iter = f_var;
      p_scope->setLastReturnLine(SM->getExpansionLineNumber(f->getLocEnd()));
      p_scope->parent_loop = NULL;
      p_scope->pos=new HPosition(f->getBody()->getLocStart());
      p_scope->s_pos=new HPosition(f->getLocStart());
      p_scope->e_pos=new HPosition(f->getLocEnd());
      CurScope->add_child(p_scope);
      CurScope = p_scope;
    }
    return true;
  }

  bool HScopeIRGen::VisitVarDecl(VarDecl *d) {
    HDecl_Mem * p_var;
    //Checkout if d is a pointer
    std::string type = d->getType().getAsString();
    if (type.find("*") != std::string::npos){
        p_var = new HDecl_Mem(d);
        if((p_var->get_type().find('['))!=std::string::npos){
            int idx_pvar=p_var->get_type().find('[');
            int num_pvar_dim=1;
            if(p_var->get_type().find('(*)'))
                num_pvar_dim++;
            while((idx_pvar=p_var->get_type().find('c',idx_pvar))<p_var->get_type().length()){
                idx_pvar++;
                num_pvar_dim++;
            }
            p_var->set_dim(num_pvar_dim);
        } else
            p_var->set_dim(0);
    }
    else
      p_var = new HDecl_Mem(d);
    CurScope->append_decl (p_var);
    if (d->hasInit()) {
      p_var->init_value = new HValueRange(d->getInit()->IgnoreImpCasts());
      HAccess_Mem * av = new HAccess_Mem(d);
      av->set_value(d->getInit()->IgnoreImpCasts());
      CurScope->addUDChain(av);
    }
    return true;
  }

  bool HScopeIRGen::VisitStmt(Stmt *S) {
    if (ReturnStmt * ret = dyn_cast<ReturnStmt> (S)) {
      HFunctionInfo * func = CurScope->get_func();
      int line = SM->getExpansionLineNumber(ret->getLocStart());
      if (func && func->last_return_line < line) 
        func->last_return_line = line; 
    }
    else if (CallExpr * call_e = dyn_cast<CallExpr> (S))
      CurScope->callers.push_back(call_e);
 
    return true;
  }

  bool HScopeIRGen::dataTraverseStmtPost (Stmt *st) {
    switch(st->getStmtClass()) {
      case clang::Stmt::CompoundStmtClass:
	if (CurScope->parent->type == 1)
	  CurScope = CurScope->parent;
	CurScope = CurScope->parent;
	break;
      case clang::Stmt::ForStmtClass: 
      case clang::Stmt::IfStmtClass: 
      case clang::Stmt::WhileStmtClass: 
      case clang::Stmt::DoStmtClass: 
      case clang::Stmt::SwitchStmtClass:
      case clang::Stmt::OMPParallelForDirectiveClass: 
      case clang::Stmt::CXXCatchStmtClass: 
      case clang::Stmt::CXXForRangeStmtClass: 
      case clang::Stmt::CXXTryStmtClass: 
      case clang::Stmt::SEHExceptStmtClass: 
      case clang::Stmt::SEHFinallyStmtClass: 
      case clang::Stmt::CapturedStmtClass: 
      case clang::Stmt::NoStmtClass:
	CurScope = CurScope->parent;
	break;
      default:
	break;
    }
  return true;
  }

  //0: One file scope; 1: function; 2: for loop; 3: do loop; 4: while loop;
  //5: if stmt; 6: switch stmt; 7: switch case;
  //8: Try; 9: Catch; 10: SEH; 11: compound; 12: others.
  //13: omp parallel for; 14: captured stmt;
  //15: if then part; 16: if else part;
  void Initialize(ASTContext &Context) {
    Ctx = &Context;
    SM = &(Ctx->getSourceManager());
  }
  bool HScopeIRGen::dataTraverseStmtPre (Stmt *st) {
    HScopeIR * p_scope ;
    HLoopInfo *p_loop = CurScope->get_loop();

    switch(st->getStmtClass()) {
      case clang::Stmt::CompoundStmtClass:
        p_scope = new HScopeIR();
	p_scope->type = 11;
	break;
      case clang::Stmt::ForStmtClass: ;
      {
          p_scope = new HLoopInfo();
          HLoopInfo *p_loop_1 = new HLoopInfo();
          Expr *f_init = dyn_cast<ForStmt>(st)->getCond();
          if(isa<BinaryOperator>(f_init)){
              Expr * value_lhs = (dyn_cast<BinaryOperator>(f_init))->getLHS()->IgnoreImpCasts();
              if(isa<DeclRefExpr>(value_lhs)){
                  HDecl_Mem * var_iter = new HDecl_Mem(dyn_cast<DeclRefExpr>(value_lhs)->getDecl());
                  if(var_iter){
                      p_scope->iter=var_iter;
                  }
              }
          }
	p_scope->type = 2;
    p_loop_1->type=2;
    p_loop_1->scope_st=st;
    p_loop_1->pos = new HPosition(st->getLocStart());
    p_loop_1->s_pos = new HPosition(st->getLocStart());
    p_loop_1->e_pos = new HPosition(st->getLocEnd());
    p_loop_1->parent=p_loop;
	p_loop->add_child(p_loop_1);
	break;
  }
      case clang::Stmt::WhileStmtClass: ;
      {
          p_scope = new HScopeIR();
          HLoopInfo *p_loop_1 = new HLoopInfo();
	p_scope->type = 4;
	p_loop_1->type=4;
	p_loop->add_child(p_loop_1);
	break;
  }
      case clang::Stmt::DoStmtClass: ;
      {
          p_scope = new HScopeIR();
          HLoopInfo *p_loop_1 = new HLoopInfo();
	p_scope->type = 3;
	p_loop_1->type=3;
	p_loop->add_child(p_loop_1);
	break;
      }
      case clang::Stmt::IfStmtClass: ;
        p_scope = new HScopeIR();
	p_scope->type = 5; 
	break;
      case clang::Stmt::SwitchStmtClass: ;
        p_scope = new HScopeIR();
	p_scope->type = 6; 
	break;
//      case clang::Stmt::SwitchCaseClass: ;
//	p_scope->type = 8; 
//	p_scope->condition = st;
//	break;
      case clang::Stmt::OMPParallelForDirectiveClass: ;
        p_scope = new HScopeIR();
	p_scope->type = 13; break;
      case clang::Stmt::CXXCatchStmtClass: ;
        p_scope = new HScopeIR();
	p_scope->type = 9; break;
      case clang::Stmt::CXXForRangeStmtClass: ;
        p_scope = new HScopeIR();
	p_scope->type = 12; break;
      case clang::Stmt::CXXTryStmtClass: ;
        p_scope = new HScopeIR();
	p_scope->type = 8; break;
      case clang::Stmt::SEHExceptStmtClass: ;
        p_scope = new HScopeIR();
	p_scope->type = 10; break;
      case clang::Stmt::SEHFinallyStmtClass: ;
        p_scope = new HScopeIR();
	p_scope->type = 12; break;
      case clang::Stmt::CapturedStmtClass: ;
        p_scope = new HScopeIR();
	p_scope->type = 14; break;
      default:
	p_scope = NULL;
	break;
    }

  if (p_scope != NULL) {
    p_scope->scope_st = st;
    p_scope->pos = new HPosition(st->getLocStart());
    p_scope->s_pos = new HPosition(st->getLocStart());
    p_scope->e_pos = new HPosition(st->getLocEnd());
    CurScope->add_child(p_scope);
    CurScope = p_scope;
  }

   return true;
  }

  //Construct Use-Def Chains.
  //And add scope for branch statements.
  bool HScopeIRGen::TraverseStmt(Stmt *S) {
    if (!S)
      return true;
  
    switch (S->getStmtClass()) {
        case clang::Stmt::CompoundAssignOperatorClass:
        {
            CompoundAssignOperator *BinOp = dyn_cast<CompoundAssignOperator>(S);
            switch (BinOp->getOpcode()) {
                case BO_MulAssign:
                case BO_DivAssign:
                case BO_AddAssign:
                case BO_SubAssign:
                {
                    CurScope->insns ++;
                    Expr * lhs = BinOp->getLHS()->IgnoreImpCasts();
                    HAccess_Mem * av = create_av_expr (lhs);
                    if (av!=NULL) {
                        av->set_value (BinOp);
                        CurScope->addUDChain(av);
                        return TraverseStmt(BinOp->getRHS());
                    }
                }
                    break;
                default:
                    break;
            }
        }
            break;
      case clang::Stmt::BinaryOperatorClass:
	{
          BinaryOperator *BinOp = dyn_cast<BinaryOperator>(S); 
          //Handle all Assign write operation here.
          if (BinOp->getOpcode() == BO_Assign) {
            CurScope->insns ++;
            Expr * lhs = BinOp->getLHS()->IgnoreImpCasts();
            HAccess_Mem * av = create_av_expr (lhs);
            if (av!=NULL) {
              av->set_value (BinOp->getRHS()->IgnoreImpCasts());
              CurScope->addUDChain(av);
              return TraverseStmt(BinOp->getRHS());
            }
          }
          if (BinOp->getOpcode() == BO_LT||BinOp->getOpcode() == BO_LE||BinOp->getOpcode() == BO_GT||BinOp->getOpcode() == BO_GE){
              CurScope->insns ++;
              Expr * lhs = BinOp->getLHS()->IgnoreImpCasts();
              HAccess_Mem * av = create_av_expr (lhs);
              if (av) {
                  int i_num=0;
                  for(auto av_internal:CurScope->access_chain){
                      if(av_internal->get_pos()->get_line()==av->get_pos()->get_line())
                          if(av_internal->get_decl()->get_name()==av->get_decl()->get_name()){
                              i_num++;
                              switch (BinOp->getOpcode()) {
                                  case BO_LT:
                                  {
                                      av_internal->set_value_max(BinOp->getRHS()->IgnoreImpCasts());
                                      std::string value_max_internal = "((";
                                      value_max_internal += av_internal->value_range->upper_boundary_str;
                                      value_max_internal += ")-1)";
                                      av_internal->value_range->upper_boundary_str=value_max_internal;
                                      break;
                                  }

                                  case BO_LE:
                                  {
                                      av_internal->set_value_max(BinOp->getRHS()->IgnoreImpCasts());
                                      std::string value_max_internal = "(";
                                      value_max_internal += av_internal->value_range->upper_boundary_str;
                                      value_max_internal += ")";
                                      av_internal->value_range->upper_boundary_str=value_max_internal;
                                      break;
                                  }
                                  case BO_GE:
                                  {
                                      av_internal->set_value_min(BinOp->getRHS()->IgnoreImpCasts());
                                      std::string value_min_internal = "(";
                                      value_min_internal += av_internal->value_range->lower_boundary_str;
                                      value_min_internal += ")";
                                      av_internal->value_range->lower_boundary_str=value_min_internal;
                                      break;
                                  }
                                  case BO_GT:
                                  {
                                      av_internal->set_value_min(BinOp->getRHS()->IgnoreImpCasts());
                                      std::string value_min_internal = "((";
                                      value_min_internal += av_internal->value_range->lower_boundary_str;
                                      value_min_internal += ")-1)";
                                      av_internal->value_range->lower_boundary_str=value_min_internal;
                                      break;
                                  }
                                  default:
                                      break;
                              }
                          }
                  }
                  if(i_num==0){
                      av->set_value (BinOp->getRHS()->IgnoreImpCasts());
                      CurScope->addUDChain(av);
                      return TraverseStmt(BinOp->getRHS());
                  }
              }
          }
	}
	break;
/*
      case clang::Stmt::UnaryOperatorClass:
	{
          UnaryOperator *UnOp = dyn_cast<UnaryOperator>(S);
          switch (UnOp->getOpcode()) {
            //Handle all unary write operation here.
            case UO_PostInc:
            case UO_PostDec:
            case UO_PreInc:
            case UO_PreDec:
	      {
                CurScope->insns ++;
                Expr * e = UnOp->getSubExpr()->IgnoreImpCasts();
                HAccess_Mem * av = create_av_expr (e);
                if (av!=NULL) {
                  av->set_value (UnOp);
                  CurScope->addUDChain(av);
                  return true;
                }
	      }
              break;

            // *ptr, read;
            // TODO: *++ptr, *(exp), *arr[], 
            case UO_Deref:
	      {
                HAccess_Mem * av = create_av_expr(UnOp);
                if (av!=NULL) {
                  CurScope->addUDChain(av);
                  return true;
                }
	      }
              break;
            case UO_AddrOf:
            case UO_Plus:
            case UO_Minus:
              break;
	    default:
	      break;
          }
	}
        break;
        */
      //TODO: Struct member read.
      case Stmt::MemberExprClass:
	{
          //MemberExpr * MemEx = dyn_cast<MemberExpr>(S); 
          return RecursiveASTVisitor<HScopeIRGen>::TraverseStmt(S);
	}
        break;
      //Array[][] Read.
      case Stmt::ArraySubscriptExprClass:
      case Stmt::DeclRefExprClass: {
          HAccess_Mem *av = create_av_expr(cast<Expr>(S));
          if (av != NULL) {
              int index_number = 0;
              for (auto av_internal:CurScope->access_chain) {
                  if((av_internal->get_decl()!=NULL)&&(av->get_decl()!=NULL))
                      if (av_internal->get_decl()->get_name() == av->get_decl()->get_name()) {
                          CurScope->addUDChain((av_internal));
                          index_number = 1;
                          break;
                      }
              }
              if (index_number == 0) {
                  HScopeIR *scope_internal = CurScope;
                  while ((scope_internal->parent->type == 2)&&(index_number==0)) {
                      scope_internal = scope_internal->parent;
                      for (auto av_internal:scope_internal->access_chain) {
                          if((av_internal->get_decl()!=NULL)&&(av->get_decl()!=NULL))
                              if (av_internal->get_decl()->get_name() == av->get_decl()->get_name()) {
                                  CurScope->addUDChain((av_internal));
                                  index_number = 1;
                                  break;
                              }
                      }

                  }
                  if (index_number == 0) {
                      CurScope->addUDChain(av);
                      return true;
                  }

              }
          }
      }
        break;
      //Create scope of true/false branch.
      case Stmt::IfStmtClass:
	{
	  IfStmt * ifs = cast<IfStmt>(S);
	  Stmt * e = ifs->getCond();
  	  TraverseStmt(e);
	  e = ifs->getInit();
  	  TraverseStmt(e);

	  e = ifs->getThen();
	  if (e) {
            HCondScope * true_scope = new HCondScope();
	    true_scope->condition = e;
	    true_scope->type = 15;
            true_scope->s_pos = new HPosition(e->getLocStart());
            true_scope->e_pos = new HPosition(e->getLocEnd());
            CurScope->add_child(true_scope);
            CurScope = true_scope;
	    TraverseStmt(e);
	    CurScope = CurScope->parent;
	  }

	  e = ifs->getElse();
	  if (e) {
            HCondScope * false_scope = new HCondScope();
	    false_scope->condition = e;
	    false_scope->type = 16;
            false_scope->s_pos = new HPosition(e->getLocStart());
            false_scope->e_pos = new HPosition(e->getLocEnd());
            CurScope->add_child(false_scope);
            CurScope = false_scope;
  	    TraverseStmt(e);
  	    CurScope = CurScope->parent;
	  }
	}
	return true;
      default:
	break;
    }

    return RecursiveASTVisitor<HScopeIRGen>::TraverseStmt(S);
  }
