//------------------------------------------------------------------------------
// C code to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#ifndef HLOOPINFO_H
#define HLOOPINFO_H
#pragma pack(8)
#include "HScopeIR.h"
#include "HKernelInfo.h"

class HLoopVar;

struct HExprStr{
  std::string str;
  Expr * e;
};


class HLoopInfo : public HScopeIR {
  private:
  bool hasCollected;
  // For these flags, 0 means undefine, -1 means false, 1 means true;
  int parallelizable; //The for loop can be parallel or not.
  int perfect; //The for loop is perfect or not.
  int canonical;//normal For loop, such as for (; var OP expr; step(var))

  struct HExprStr end_cond;
  struct HExprStr start_value;
  void collectVars();
  void addVar(HAccess_Mem *);
  int direction; //-1: decrease; 1: increase.

  public:
  struct HExprStr iter_num;
  //All var which accessed in this loop.
  HLoopInfo() {hasCollected=false;parallelizable=0;perfect=0;canonical=0;}
    ~HLoopInfo() {}
  std::vector<HLoopVar *> loopVars;
  std::vector<HLoopInfo *> children_loops;
  HLoopInfo * parent_loop;


  bool isPerfect();
  bool isParallelizable();
  bool isCanonical();
  Stmt * loopStmt() {return get_stmt();}
  std::string get_start_value_str() {return start_value.str;}
  std::string get_iter_num_str() {return iter_num.str;}
  std::string get_iter_name () {return iter->get_name();}
  void set_start_value_str(std::string name) {start_value.str=name;}
//  void set_end_cond_str(std::string name){end_cond.str=name;}
  HDecl_Var * get_iter() {return iter;}
  ValueDecl * get_iter_stmt () {return iter->get_decl_stmt();}
  void add_child(HLoopInfo * l);
};

class HLoopVar{
  private:
  // For these flags, 0 means undefine, -1 means false, 1 means true;
  int inductive;
  int cansplit;

  //TODO: wait for further dependence analysis need.
  //std::vector<std::pair<Access_Var *, Access_Var *>> dependenceMap;

  HDecl_Mem * var_decl;

  public:
  HLoopInfo * loop;
  std::vector<HAccess_Mem *> avInloop; //All access in This loop.
  bool tryMerge(HLoopVar * lv);
  void addAV(HAccess_Mem *);
  bool isInitialized();
  HLoopVar * copy();
  //If this mem/array can be partitioned in this loop.
  bool canSplit();

  void setInductive() {inductive = 1;}
  void set_var_decl (HDecl_Mem * vd) {var_decl = vd;}

  bool isInductive() {return (inductive == 1);}
  std::string get_name() {return var_decl->get_name();}
  std::string get_type() {return var_decl->get_type();}
  int get_category() {return var_decl->get_category();}
  std::string get_total_size () {return ((HDecl_Mem *)var_decl)->get_total_size()->get_upper_str();}
  int get_dim () {return ((HDecl_Mem *)var_decl)->get_dim();}
  int access_type() {return var_decl->access_type((HScopeIR*)loop);}
  HPosition * get_pos() {return var_decl->get_pos();}
  bool isIter() {return var_decl == loop->get_iter();}
  HDecl_Mem * get_var_decl () {return var_decl;}
  HLoopVar() {inductive = 0; cansplit=0;}
    ~HLoopVar() {}
};

#endif
