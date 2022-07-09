//------------------------------------------------------------------------------
// C code to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#include "HLoopInfo.h"

bool HLoopVar::tryMerge(HLoopVar * lv) {
  if (lv->var_decl != var_decl)
    return false;

  for (auto& av : lv->avInloop)
    addAV(av);

  return true;
}

void HLoopVar::addAV(HAccess_Mem *av) {

  //Check if already exist.
  for (auto& ac_va : avInloop)
    if (av == ac_va)
      return;

  avInloop.push_back(av);
}

//FIXME:Have not consider branch control flow and array.
bool HLoopVar::isInitialized() {
  if(avInloop.empty())
      return false;
  HAccess_Var * first = avInloop.front();
  for (auto&av : avInloop) {
    if (*(first->get_pos()) > *(av->get_pos()))
      first = av;
  }

  if (first->isMem() == false && first->isWrite() == true)
    return true;

  return false;
}

HLoopVar * HLoopVar::copy() {
  HLoopVar * ret = new HLoopVar();
  ret->var_decl = var_decl;
  ret->avInloop = avInloop;
  return ret;
}

bool HLoopVar::canSplit() {
  if (cansplit != 0)
    return (cansplit == 1);

  cansplit = -1;
  if (var_decl->is_Mem() == false)
    return false;
  HAccess_Mem *pre_am = (HAccess_Mem *)avInloop[0];
  HAccess_Mem *cur_am;

  int sum = 0;
  int has_changed=0;
    /*
  Expr * e = pre_am->get_index(0);
  if (isVarAffineRelatedExpr(e, loop->get_iter_stmt()) == true)
     sum ++;
    */

  for (int i = 0; i < pre_am->get_dim(); i++) {
    Expr * e = pre_am->get_index(i);
    if (isVarAffineRelatedExpr(e, loop->get_iter_stmt()) == true)
      sum ++;
  }

  if (sum != 1)
    return false;

  for (unsigned int i = 1; i < avInloop.size(); i++) {
    cur_am = (HAccess_Mem *)avInloop[i];
    if (pre_am->get_dim() != cur_am->get_dim())
      return false;
    for (int j = 0; j < cur_am->get_dim(); j++) {
      if (compareAlgebraExpr(pre_am->get_index(j), cur_am->get_index(j)) != 0)
          has_changed++;
      if(has_changed!=0)
	    return false;
    }
  }

  cansplit = 1;
  return true;
}

// For these flags, 0 means undefine, -1 means false, 1 means true;
bool HLoopInfo::isPerfect() {
  if (perfect != 0)
    return (perfect == 1);

  perfect = 1;
  for (auto & child : children_loops) {
    if (!child->isPerfect()) {
      perfect = -1;
      return false;
    }
  }

  //Traverse all scopes, if there are any branchs except loop, 
  std::deque<HScopeIR *> scopeQue;
  scopeQue.push_back(this);
  while (perfect == 1 && scopeQue.empty() == false) {
    HScopeIR * scope = scopeQue.front();
    scopeQue.pop_front();
    for (auto& child : scope->children) {
      switch (child->get_type()) {
	case 2:
	case 3:
	case 4:
	  break;
	case 5:
	case 6:
	case 7:
	case 15:
	case 16:
	  perfect = -1;
	  break;
	default:
	  scopeQue.push_back(child);
      }
    }
  }

  if (perfect == 1)
    return true;

  return false;
}

bool HLoopInfo::isParallelizable() {

  //TODO: We can only parallel canonical loop right now.
  if (!isCanonical()) {
    parallelizable = -1;
    return false;
  }

  //We omit the loop which has call statement.
  if (get_call_num() > 0) {
    parallelizable = -1;
    return false;
  }

  if (parallelizable != 0)
    return (parallelizable == 1);

  parallelizable= 1;

  //If the index of array is not loop inductive or is a scalar, then var only be read or has been initialized in loop, otherwise will have WAW/RAW/WAR dependence.
  //Else is var is loop inductive, which means var is memory access and with an index.
    //If var has only one index, there are no dependence between loop iterations.
    //Else if var has two or more indices, then var only be read or we are sure these indices are always not equal in the range of loop.
      //TODO:Right now we do not consider the second condition.
  //Collect all var accessed in this loop.
  collectVars();
  for(auto & loop_var_internal:loopVars)
      int i_internal=1;
  for (auto& loop_var : loopVars) {
    //Except the loop iterator.
    if (loop_var->isIter())
      continue;
    //Except the scalar variable which has been initialized in loop.
    if(loop_var->isInitialized() == true)
      continue;

    int num_index = 1;
    bool has_write = false;
    int iter_related = 0;

    if (loop_var->get_var_decl()->is_Mem()) {
      HAccess_Mem * am_base = (HAccess_Mem *)(loop_var->avInloop[0]);
      for (auto& av : loop_var->avInloop) {
        if (am_base->get_dim() != ((HAccess_Mem *)av)->get_dim())
          num_index ++;
        else {
          for (int k = 0; k < ((HAccess_Mem *)av)->get_dim(); k ++) {
            if (compareAlgebraExpr(am_base->get_index(k), ((HAccess_Mem *)av)->get_index(k)) !=0) {
              num_index ++;
              break;
            }
          }
        }

        for (int k = 0; k < ((HAccess_Mem *)av)->get_dim(); k ++) {
          if (isVarRelatedExpr(((HAccess_Mem *)av)->get_index(k), iter->get_decl_stmt())) {
            iter_related ++;
            break;
          }
        }
      }
    }

    for (auto& av : loop_var->avInloop) {
      if (av->isWrite()) 
        has_write = true;
    }

    if (iter_related > 0)
	loop_var->setInductive();

    if (iter_related > 0 && has_write && num_index > 1)
        parallelizable = -1;
    else if (iter_related == 0 && has_write)
	parallelizable = 1;

    if (parallelizable != 1)
      break;
  }

  if (parallelizable == 1)
    return true;

  return false;
}

void analysis_rhs_var(BinaryOperator *rhs,HLoopInfo *loop){
    if(isa<BinaryOperator>(rhs->getLHS()->IgnoreImpCasts()))
        analysis_rhs_var((dyn_cast<BinaryOperator>(rhs->getLHS()->IgnoreImpCasts())),loop);
    if(isa<ArraySubscriptExpr>(rhs->getLHS()->IgnoreImpCasts())){
        HAccess_Mem * av_infor = new HAccess_Mem(dyn_cast<ArraySubscriptExpr>(rhs->getLHS()->IgnoreImpCasts()));
        if(!loop->find_var(av_infor))
        {
            av_infor->scope=pos_to_scope(*loop->pos);
            loop->access_chain.push_back(av_infor);
        }
    }
    if(isa<BinaryOperator>(rhs->getRHS()->IgnoreImpCasts()))
        analysis_rhs_var((dyn_cast<BinaryOperator>(rhs->getRHS()->IgnoreImpCasts())),loop);
    if(isa<ParenExpr>(rhs->getRHS()->IgnoreImpCasts())){
        if(isa<BinaryOperator>(dyn_cast<ParenExpr>(rhs->getRHS()->IgnoreImpCasts())->getSubExpr())){
           analysis_rhs_var(dyn_cast<BinaryOperator>(dyn_cast<ParenExpr>(rhs->getRHS()->IgnoreImpCasts())->getSubExpr()),loop);
        }
    }
    if(isa<ArraySubscriptExpr>(rhs->getRHS()->IgnoreImpCasts())){
        HAccess_Mem * av_infor = new HAccess_Mem(dyn_cast<ArraySubscriptExpr>(rhs->getRHS()->IgnoreImpCasts()));
        if(!loop->find_var(av_infor))
        {
            av_infor->scope=pos_to_scope(*loop->pos);
            loop->access_chain.push_back(av_infor);
        }
    }

}
void find_var_infor(ForStmt *f,HLoopInfo *loop){
    Stmt *s_internal = f->getBody();
    while (isa<ForStmt>(s_internal))
        s_internal=(dyn_cast<ForStmt>(s_internal))->getBody();
    if(isa<CompoundStmt>(s_internal)){
        for (Stmt::child_iterator it = s_internal->child_begin(); it != s_internal->child_end(); ++it){
            Stmt *it_i=*it;
            if(isa<BinaryOperator>(it_i)){
                Expr *lhs = (dyn_cast<BinaryOperator>(it_i))->getLHS()->IgnoreImpCasts();
                if(isa<ArraySubscriptExpr>(lhs)){
                    HAccess_Mem * av_infor = new HAccess_Mem(dyn_cast<ArraySubscriptExpr>(lhs));
//                    if(dyn_cast<BinaryOperator>(it_i)->getOpcode()==BO_Assign)
//                        av_infor->set_value(lhs);
                    if(!loop->find_var(av_infor)){
                        av_infor->scope=pos_to_scope(*loop->pos);
                        loop->access_chain.push_back(av_infor);
                    }
                }
            }
            if(isa<ForStmt>(it_i)){
                find_var_infor(dyn_cast<ForStmt>(it_i),loop);
            }
        }
    }
    if(isa<BinaryOperator>(s_internal)){
        Expr *lhs = (dyn_cast<BinaryOperator>(s_internal))->getLHS()->IgnoreImpCasts();
        if(isa<ArraySubscriptExpr>(lhs)){
            HAccess_Mem * av_infor = new HAccess_Mem(dyn_cast<ArraySubscriptExpr>(lhs));
            if(!loop->find_var(av_infor))
            {
                av_infor->scope=pos_to_scope(*loop->pos);
                loop->access_chain.push_back(av_infor);
            }

        }
        Expr *rhs = (dyn_cast<BinaryOperator>(s_internal))->getRHS()->IgnoreImpCasts();
        if(isa<BinaryOperator>(rhs))
            analysis_rhs_var((dyn_cast<BinaryOperator>(rhs)),loop);
        if(isa<ArraySubscriptExpr>(rhs)){
            HAccess_Mem * av_infor = new HAccess_Mem(dyn_cast<ArraySubscriptExpr>(rhs));
            if(!loop->find_var(av_infor))
            {
                av_infor->scope=pos_to_scope(*loop->pos);
                loop->access_chain.push_back(av_infor);
            }
        }
    }
}

void add_var_infor(HScopeIR * scope,HLoopInfo * loop){
    for(auto scope_infor_child:scope->children){
        for(auto av_infor:scope_infor_child->access_chain)
            if(!loop->find_var(av_infor))
                loop->access_chain.push_back(av_infor);
        if(!scope_infor_child->children.empty())
                add_var_infor(scope_infor_child,loop);
    }
}
//FIXME:This function still need to be completed.
bool HLoopInfo::isCanonical() {
  if (canonical != 0)
    return (canonical == 1);
  canonical = -1;
  if (get_type() == 2) {
    if (ForStmt * f = dyn_cast<ForStmt>(get_stmt())) {
      Expr * step = f->getInc();
      if (step == NULL)
	return false;
      find_var_infor(f,this);
      HScopeIR * scope_infor = pos_to_scope(*this->pos);
      add_var_infor(scope_infor,this);
      if (BinaryOperator *BinOp = dyn_cast<BinaryOperator>(step)) {
        switch (BinOp->getOpcode()) {
          case BO_MulAssign:
          case BO_DivAssign:
          case BO_AddAssign:
          case BO_SubAssign:
	    {
	      Expr * e = BinOp->getLHS()->IgnoreImpCasts();
	      if (DeclRefExpr * d = dyn_cast<DeclRefExpr> (e)) {
	        iter = find_var(d->getDecl());
	        if (iter)
	          canonical = 1;
	      }
	    }
	    break;
	  default:
	    break;
        }
      }
      else if (UnaryOperator * UnOp = dyn_cast<UnaryOperator>(step)) {
        switch (UnOp->getOpcode()) {
          case UO_PostInc:
          case UO_PostDec:
          case UO_PreInc:
          case UO_PreDec:
	    {
              Expr * e = UnOp->getSubExpr()->IgnoreImpCasts();
	      if (DeclRefExpr * d = dyn_cast<DeclRefExpr> (e)) {
	        iter = find_var(d->getDecl());
	        if (iter)
	          canonical = 1;
	      }
	    }
	    break;
	  default:
	    break;
	}
      }
    }
  }

  if (canonical == 1)
    return true;

  return false;
}

void HLoopInfo::collectVars() {

  if (hasCollected)
    return;

  //Traverse all scopes, if there are any branchs except loop, 
  std::deque<HScopeIR *> scopeQue;
  scopeQue.push_back(this);
  while (scopeQue.empty() == false) {
    HScopeIR * scope = scopeQue.front();
    scopeQue.pop_front();

    for (auto& av: scope->access_chain)
      addVar(av);

    for (auto& child : scope->children) {
      switch (child->get_type()) {
	case 2:
	case 3:
	case 4:
	  break;
	default:
	  scopeQue.push_back(child);
      }
    }
  }

  for (auto& child : children_loops) {
    child->collectVars();
    for (auto& child_lv : child->loopVars) {
      bool merged = false;
      //Do not collect variables which are defined in loop body.
      if (isInside(child_lv->get_var_decl()->get_scope()) == true)
	continue;
      HLoopVar * new_lv = child_lv->copy();
      for (auto& lv : loopVars) {
	merged = lv->tryMerge(new_lv);
	if (merged == true)
	  break;
      }
      if (merged == false)
	loopVars.push_back(new_lv);
    }
  }

  hasCollected = true;
}

void HLoopInfo::addVar(HAccess_Mem *av) {
  for (auto& lv : loopVars) {
    if (av->get_decl() == lv->get_var_decl()) {
      lv->addAV(av);
      return;
    }
  }

  if (isInside(av->get_decl()->get_scope()) == true)
    return;

  HLoopVar * lv = new HLoopVar();
  /*
  HLoopVar * lv_ir = new HLoopVar();
  HScopeIR * scope_ir;
  int idx_ir;
  scope_ir = av->get_decl()->get_scope();
  while(scope_ir->get_type()!=1){
    scope_ir = scope_ir->parent;
  }
  if(idx_ir==decl_chain.size()){

  } else
  {
      HAccess_Mem * av_ir = new HAccess_Mem();
      for(auto decl_ir : scope_ir->decl_chain)
      {
          if(decl_ir->get_category()==2||decl_ir->get_category()==4){
              av_ir->decl_var=decl_ir;
              av_ir->scope=scope_ir;
              av_ir->pos=scope_ir->pos;
              if(decl_ir->is_Mem())
                  av_ir->is_mem=true;
              else
                  av_ir->is_mem= false;
              lv_ir->addAV(av_ir);
              lv_ir->set_var_decl(decl_ir);
              lv_ir->loop=scope_ir->get_loop();
              loopVars.push_back(lv_ir);
          }
      }
      idx_ir=decl_chain.size();
  }
   */
  lv->set_var_decl(av->get_decl());
  lv->loop=av->scope->get_loop();
  lv->addAV(av);
  loopVars.push_back(lv);
}

void HLoopInfo::add_child(HLoopInfo * l) {
  if (l == NULL)
    return;
  l->parent_loop = this;
  children_loops.push_back(l);
}
