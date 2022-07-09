//------------------------------------------------------------------------------
// C code to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

//===----------------------------------------------------------------------===//
//
// ScopeIR: generate ScopeIR from AST. 
//
//===----------------------------------------------------------------------===//
#ifndef HSCOPEIR_H
#define HSCOPEIR_H

#include "ClangHeader.h"
#pragma pack(8)
class HScopeIR;
class HAccess_Var;
class HDecl_Var;
class HLoopInfo;
class HFunctionInfo;
class HExprSerializer;

class HPosition {
  private:
  unsigned int col;
  unsigned int line;

  public:

  HPosition() {}
  HPosition(SourceLocation loc);
  ~HPosition() {}
  bool operator<(const HPosition & pos);
  bool operator>(const HPosition & pos);
  bool operator>=(const HPosition & pos);
  bool operator<=(const HPosition & pos);
  bool operator==(const HPosition & pos);
  HPosition operator=(const HPosition & pos);
  unsigned int get_line() {return line;}
  unsigned int get_col() {return col;}
  void set_col(unsigned col){this->col=col;}
  void set_line(unsigned line){this->line=line;}
};

class HValueRange {
  private:

public:
    //From conditional expression in if or for,  we can get the lower or upper boundary.
    Expr * value;
    std::string lower_boundary_str;
    std::string upper_boundary_str;
    //-2:undefined; -1: Expr; 0: integer; 1: float;
    int type=0;
  HValueRange();
  HValueRange(Expr * e);
  ~HValueRange();

  void set_value (Expr *e);
  Expr * get_value() {return value;}
  std::string get_lower_str () {return lower_boundary_str;}
  std::string get_upper_str () {return upper_boundary_str;}
  int get_type() {return type;}
};

class HDecl_Var{
  private:

  // 0 means function decl; 
  // 1 means scalar function parameter; 
  // 2 means pointer function parameter; 
  // 3 means scalar variable;
  // 4 means pointer variable.
  int category; 
  std::string name;
  std::string type; // Type of variable.
  HPosition *pos;
  std::vector <HAccess_Var *> access_chain;
  public:
  ValueDecl * decl_stmt;
  HDecl_Var();
  HDecl_Var(ValueDecl * decl);
  ~HDecl_Var();
  HScopeIR * scope;
  HValueRange * init_value;
  void append_access( HAccess_Var * av) {access_chain.push_back(av);}

  HAccess_Var * find_last_access(HPosition &pos, bool isWrite);
  HAccess_Var * find_next_access(HPosition &pos, bool isWrite);
  HPosition * get_pos() {return pos;}
  std::string get_type() {return type;}
  std::string get_name() {return name;}
  int get_category() {return category;}
  //0: no access in ScopeIR s; 1: only read; 2: only write; 3: read and write.
  int access_type (HScopeIR *s);
  bool is_Mem() {return (category == 2 || category == 4);}
  ValueDecl * get_decl_stmt () {return decl_stmt;}
  HScopeIR * get_scope() {return scope;}
};

class HDecl_Mem : public HDecl_Var {
  private:
  HValueRange * elem_num;
  HValueRange * total_size;
  int dim;

  public:
  HDecl_Mem() {}
  HDecl_Mem(ValueDecl * decl) : HDecl_Var(decl) {}
  ~HDecl_Mem();


  void set_elem_num(HValueRange *v) {elem_num = v;}
  void set_total_size(HValueRange *v) {total_size = v;}
  void set_dim (int d) {dim = d;}

  HValueRange * get_elem_num () {return elem_num;}
  HValueRange * get_total_size () {return total_size;}
  int get_dim () {return dim;}
};

class HAccess_Var {
  private:
  public:
  int type; // 0: normal var; 1: means loop var; 2: loop inductive var; 3: branch var.
  HDecl_Mem * decl_var;
  HScopeIR * scope;
  bool is_mem;
  HValueRange *value_range; // For write, and distinguish read or write;
  HPosition *pos;
  HAccess_Var(DeclRefExpr * ref);
  HAccess_Var(VarDecl * d);
  HAccess_Var();
  ~HAccess_Var();
  HAccess_Var * find_last_access(ValueDecl * decl_stmt, bool isWrite);
  HAccess_Var * find_next_access(ValueDecl * decl_stmt, bool isWrite);
  HDecl_Mem * get_decl(){return decl_var;}
  bool isValid() {return (decl_var != NULL);}
  void set_value (Expr *e);
  void set_ismem () {is_mem = true;}
  void set_type (int t) {type = t;}
  void set_value_max(Expr *e);
  void set_value_min(Expr *e);
  HValueRange * get_value () {return value_range;}
  
  bool isWrite() {return value_range ? true: false;}
  bool isMem() {return is_mem;}
  HScopeIR * get_scope() {return scope;}
  HPosition * get_pos() {return pos;}
};

class HAccess_Mem : public HAccess_Var {
  private:
  std::vector <Expr *> index;//For array or memory pointer
  std::vector <HValueRange *> index_value;//For array or memory pointer
  void set_index(Expr *e);
  public:
  HAccess_Mem();
  //TODO: the mem pointer need to be handled.
  HAccess_Mem(ArraySubscriptExpr * ArrEx);
  HAccess_Mem(DeclRefExpr * ref);
  HAccess_Mem(VarDecl * d);
  ~HAccess_Mem();
  int get_dim () {return index.size();}
  Expr * get_index(int i) {return index[i];}
  HValueRange * get_index_value(int i) {return index_value[i];}
};

class HScopeIR {
  private:




  //Start and End position of this scope;


  //-1: Program scope;
  //0: One file scope; 1: function; 2: for loop; 3: do loop; 4: while loop;
  //5: if stmt; 6: switch stmt; 7: switch case;
  //8: Try; 9: Catch; 10: SEH; 11: compound; 12: others.
  //13: omp parallel for; 14: captured stmt;
  //15: if then part; 16: if else part;


  public:
  std::vector <HDecl_Mem *> decl_chain;
  HPosition *s_pos;
  HPosition *pos;
  HPosition *e_pos;
  std::vector<CallExpr *> callers;
  std::vector <HAccess_Mem *> access_chain;
  std::vector<HScopeIR *> children;
  unsigned int insns; //The total number of insns in this scope.
  HScopeIR * parent;
  int type;
  Stmt * scope_st;
  HDecl_Var * iter;
  //The statement which generates this scope.
  HScopeIR();
  ~HScopeIR();
  //Find the decl info in this scope or outside scope.
  HDecl_Mem * find_var(ValueDecl * decl_stmt);
  bool find_var(HAccess_Mem * acc_var);
  HDecl_Mem * get_var(unsigned int n) {return decl_chain[n];}
  void add_child(HScopeIR * scope);

  void addUDChain(HAccess_Mem * acc_var);
  void append_decl(HDecl_Mem * dv) {decl_chain.push_back(dv); dv->scope = this;}
  void set_insns (int n) {insns = n;}
  void set_type (int t) {type = t;}

  HPosition * get_pos () {return pos;}
  HPosition * get_start_pos () {return s_pos;}
  HPosition * get_end_pos () {return e_pos;}
  int get_line() {return pos->get_line();}
  int get_start_line() {return s_pos->get_line();}
  int get_end_line() {return e_pos->get_line();}

  void dump(); 
  unsigned int get_call_num();
  unsigned int get_insns_num();

  Stmt * get_stmt() {return scope_st;}
  unsigned int get_type() {return type;}
  //Is s in this scope.
  bool isInside(HScopeIR *s);
  HFunctionInfo  * get_func();
  HLoopInfo * get_loop();
};

class HCondScope : public HScopeIR {
  public:
  HCondScope() {}
  ~HCondScope() {}

  Stmt * condition ; //The condition statement which is true.
};

extern HScopeIR * TopScope;
extern HScopeIR * CurScope;

//Compare two algebra expression has the same value in this context.
//0: equal.
int compareAlgebraExpr(Expr * e1, Expr * e2);
//Find if the Algebra expression is related to Var, it will trace the compute chain.
bool isVarRelatedExpr(Expr * e, ValueDecl * v);
bool isVarAffineRelatedExpr(Expr * e, ValueDecl * v);

//Find the scope according to the position.
HScopeIR * pos_to_scope (HPosition &p);

//Value Evaluate
int evaluate (Expr *e, std::string &min_str, std::string &max_str);
int evaluateBinop (BinaryOperator *op, std::string &min_str, std::string &max_str);
int evaluateUnop (UnaryOperator *up, std::string &min_str, std::string &max_str);
int evaluateParen (ParenExpr *pe, std::string &min_str, std::string &max_str);
int evaluateDeclRef (DeclRefExpr *ref, std::string &min_str, std::string &max_str);

#endif
