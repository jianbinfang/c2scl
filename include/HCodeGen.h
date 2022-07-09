//===------------------------ HCodeGen.h --------------------------===//
//
// This file is distributed under the Universidade Federal de Minas Gerais -
// UFMG Open Source License. See LICENSE.TXT for details.
//
// Copyright (C) 2018  Peng Zhang 
//
//===----------------------------------------------------------------------===//
//
// HCodeGen: according the instructions, generate Streamed pragrom from
// C code.
//
//===----------------------------------------------------------------------===//
#ifndef HCODEGEN_H
#define HCODEGEN_H

#include <vector>
#include <string>

#include "HKernelInfo.h"

class HCodeGen {

  private:

  //===---------------------------------------------------------------------===
  //                             Names 
  //===---------------------------------------------------------------------===
  std::string InputFile;
  std::string DevFile;
  std::string HostFile;
  std::string KernelName;
  std::string DevLibName;
  std::vector<unsigned > kernel_dims;

  //===---------------------------------------------------------------------===
  //                            Parameters for hStreams generator 
  //===---------------------------------------------------------------------===
  std::vector<unsigned > kernel_args;
  std::vector<unsigned > val_num;
  std::vector<unsigned > pointer_num;

  /*
  unsigned kernel_args;
  unsigned val_num;
  unsigned pointer_num;
   */
  std::vector<std::vector<hvar_decl>> arg_decls_chain;
  std::vector<std::vector<hvar_decl>> var_decls_chain;
  std::vector<std::vector<hvar_decl>> fix_decls_chain;
  /*
  std::vector<hvar_decl> arg_decls;//all of kernel args.
  std::vector<hvar_decl> var_decls;//local define vars in kernel.
  std::vector<hvar_decl> fix_decls;//args that independ to streams and tasks.
   */
  std::vector<hvar_decl> writed_fix_decls;

  unsigned logical_streams;

  std::vector<unsigned > task_blocks;
  std::vector<std::string > loop_var;
  std::vector<std::string > length_var_name;
  std::vector<std::string > start_index_str;

    /*
    unsigned task_blocks;
    //The total task size.

    std::string loop_var;
    std::string length_var_name;
    std::string start_index_str;
     */

  std::vector<std::vector<hmem_xfer>> pre_xfers_chain;
  std::vector<std::vector<hmem_xfer>> mem_bufs_chain;
  std::vector<std::vector<hmem_xfer>> h2d_xfers_chain;
  std::vector<std::vector<hmem_xfer>> d2h_xfers_chain;
  std::vector<std::vector<hmem_xfer>> post_xfers_chain;
  std::vector<hmem_xfer> created_xfers_chain;
  std::vector<hmem_xfer> writed_xfers_chain;
  std::vector<hmem_xfer> readed_xfers_chain;
  std::vector<hmem_xfer> realesed_xfers_chain;
/*
    //mem xfers that is before all kernel execution.
  std::vector<hmem_xfer> pre_xfers;
  //all mem bufs that need be created.
  std::vector<hmem_xfer> mem_bufs;
  //xfers that from host to device.
  std::vector<hmem_xfer> h2d_xfers;
  //xfers that from device to host.
  std::vector<hmem_xfer> d2h_xfers;
  //mem xfers that is transfer after all kernel execution.
  std::vector<hmem_xfer> post_xfers;

  std::vector<hmem_xfer> has_bufs;
*/
  std::vector<unsigned > replace_line;
  std::vector<unsigned > enter_loop;
  std::vector<unsigned > enter_loop2;
  std::vector<unsigned > exit_loop;
  std::vector<unsigned > init_site;
  std::vector<unsigned > init_site_e;
  std::vector<unsigned > finish_site;
  std::vector<unsigned > create_mem_site;
  std::vector<Array_rw_chain > arr_chain;

/*
  unsigned replace_line;
  unsigned enter_loop;
  unsigned exit_loop;
  unsigned init_site;
  unsigned init_site_e;
  unsigned finish_site;
  unsigned create_mem_site;
  */
  unsigned include_insert_site;
  unsigned cuda_kernel_site;

  //===---------------------------------------------------------------------===
  //                           Utility functions 
  //===---------------------------------------------------------------------===
  bool is_include (std::string line);
  bool in_loop (unsigned LineNo,unsigned loop_id);
  bool in_loop_kernel(unsigned LineNo,unsigned loop_id);
  std::string generateCUKernelCode();


  public:

  HCodeGen() ;
  ~HCodeGen() ;
  // To print Information in source file.
  void generateDevFile ();
  void generateHostFile ();
  void set_InputFile(std::string input);
  void set_kernel_dim(unsigned dim);
  void set_enter_loop (unsigned LineNo);
  void set_enter_loop2 (unsigned LineNo);
  void set_exit_loop (unsigned LineNo);
  void set_init_site (unsigned LineNo);
  void set_init_site_e (unsigned LineNo);
  void set_finish_site (unsigned LineNo);
  void set_create_mem_site (unsigned LineNo);
  void add_kernel_arg(struct hvar_decl var,unsigned i);
  void add_local_var(struct hvar_decl var,unsigned i);
  void set_replace_line(unsigned n);
  void set_logical_streams(unsigned n);
  void set_task_blocks(unsigned n);
  void set_length_var(std::string name);
  void set_loop_var(std::string name);
  void set_start_index(std::string name);
  void add_mem_xfer(struct hmem_xfer m,unsigned i);
  void set_include_site (unsigned LineNo);
  void set_cuda_site (unsigned LineNo);
  void push_back_args(unsigned args){kernel_args.push_back(args);}
  void push_back_valnum(unsigned valnum){val_num.push_back(valnum);}
  void push_back_pointernum(unsigned pointernum){pointer_num.push_back(pointernum);}
  void set_arr_chain(Array_rw_chain arr_rw_chain);
  void generateOCLDevFile ();
  void generateOCLHostFile ();
  void generateOCLHeadFile ();

  void generateCUDAFile ();
};


//===------------------------ HCodeGen.h --------------------------===//
#endif
