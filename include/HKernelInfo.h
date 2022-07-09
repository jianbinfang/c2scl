//------------------------------------------------------------------------------
// OpenMP to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#ifndef HKERNELINFO_H
#define HKERNELINFO_H

#include <sstream>
#include <string>
#include <iostream>

#include "ClangHeader.h"

struct hvar_decl {
  std::string type_name;
  std::string var_name;
  unsigned id;//the order in kernel args.
  unsigned type;//00=value + fix, 01=point+fix, 10=value+var, 11=p+v.
};

//TODO: The overlap need to be handled
struct hmem_xfer {
  std::string  buf_name;
  std::string size_string;
  std::string type_name;
  std::string elem_type;
  unsigned dim;//The dim of arry;
  unsigned type;//x1:pre_xfers; x1x:h2d; 1xx:d2h; 1xxx: post_xfer
};

struct HFile_Info {
  int last_include = 0;
  int start_function = 0;
  int return_site = 0;
  int start_function_e=0;
};
struct Array_rw_chain
{
    std::string name;
    unsigned start_place;
    unsigned end_place;
};
struct HKernel_Info {
  unsigned enter_loop;
  unsigned exit_loop;
  unsigned init_site ;
  unsigned init_site_e;
  unsigned finish_site;
  unsigned create_mem_site;
  unsigned replace_line;
  unsigned last_include;
  unsigned enter_loop2;
  std::vector<hmem_xfer> mem_bufs;
  std::vector<hvar_decl> val_parms;
  std::vector<hvar_decl> pointer_parms;
  std::vector<hvar_decl> local_parms;

  //ValueDecl * loop_index;

  std::string length_var;
  std::string loop_var;
  //The init value of loop index may not be 0!
  std::string start_index;

  //The omp parallel for iteration index variable declaration
  //The compute instructions.
  unsigned int insns;
};

#endif
