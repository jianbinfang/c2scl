//===------------------------ HCodeGen.cpp --------------------------===//

#include <fstream>
#include <string.h>
//#include <iostream>
//#include <sstream>
#include <stdio.h>
#include <sys/io.h>
#include <unistd.h>
#include "HCodeGen.h"
#include <stdlib.h>

#define CarriageReturn 13

using namespace std;

#define DEBUG_TYPE "HCodeGen"



HCodeGen::HCodeGen() {
    /*
  kernel_args = 0;
  val_num = 0;
  pointer_num = 0;
  logical_streams = 0;
  task_blocks = 0;
  replace_line = 0;
  enter_loop = 0;
  exit_loop = 0;
  init_site = 0;
  finish_site = 0;
     */
}

HCodeGen::~HCodeGen() {
}

void HCodeGen::set_InputFile(std::string inputFile) {
  std::string key (".");
  std::size_t found = inputFile.rfind(key);

  InputFile = inputFile;

  DevFile = inputFile;
  DevFile.replace(found, key.length(), "_dev.");

  HostFile = inputFile;
  HostFile.replace(found, key.length(), "_host.");

  DevLibName = "kernel.so";
  KernelName = "kernel";
}

bool HCodeGen::is_include (std::string line) {
  std::size_t found1 = line.find_first_not_of(" ");
  std::size_t found2 = line.find_first_not_of("\t");
  std::size_t found = found1 > found2 ? found1 : found2;
  if (found < line.size())
    return (line.compare(found, 8, "#include") == 0);
  else
    return false;
}

bool HCodeGen::in_loop (unsigned LineNo,unsigned loop_id) {
  return (LineNo > enter_loop[loop_id] && LineNo < exit_loop[loop_id]);
}
bool HCodeGen::in_loop_kernel (unsigned LineNo,unsigned loop_id) {
    return (LineNo > enter_loop[loop_id] && LineNo <= exit_loop[loop_id]);
}
void HCodeGen::set_kernel_dim(unsigned dim) {
    kernel_dims.push_back(dim);
}
void HCodeGen::set_enter_loop(unsigned LineNo) {
  enter_loop.push_back(LineNo);
}
void HCodeGen::set_enter_loop2(unsigned LineNo) {
    enter_loop2.push_back(LineNo);
}
void HCodeGen::set_exit_loop(unsigned LineNo) {
  exit_loop.push_back(LineNo);
}

void HCodeGen::set_init_site(unsigned LineNo) {
  init_site.push_back(LineNo);
}

void HCodeGen::set_init_site_e(unsigned LineNo) {
  init_site_e.push_back(LineNo);
}

void HCodeGen::set_finish_site(unsigned LineNo) {
  finish_site.push_back(LineNo);
}
void HCodeGen::set_create_mem_site(unsigned LineNo) {
  create_mem_site.push_back(LineNo);
}

void HCodeGen::add_kernel_arg(struct hvar_decl var,unsigned i) {
  if(arg_decls_chain.size()<=i){
    std::vector<hvar_decl> arg_decls_init;
    arg_decls_chain.push_back(arg_decls_init);
  }
  if(fix_decls_chain.size()<=i){
    std::vector<hvar_decl> fix_decls_init;
    fix_decls_chain.push_back(fix_decls_init);
  }

  var.id = kernel_args[i];
  arg_decls_chain[i].push_back(var);
  kernel_args[i] ++;
  //Check the var is value argument or pointer argument.
  if (var.type & 1)
    pointer_num[i] ++;
  else
    val_num[i] ++;

  //Check the var is independ to streams and tasks or not.

//  if ((var.type & 2) == 0)
    fix_decls_chain[i].push_back(var);

}

void HCodeGen::add_local_var(struct hvar_decl var,unsigned i) {
  if(var_decls_chain.size()<=i){
    std::vector<hvar_decl> var_decls_init;
    var_decls_chain.push_back(var_decls_init);
  }
  var_decls_chain[i].push_back(var);
}

void HCodeGen::set_replace_line(unsigned n) {
  replace_line.push_back(n);
}

void HCodeGen::set_logical_streams(unsigned n) {
  logical_streams = n;
}

void HCodeGen::set_task_blocks(unsigned n) {
  task_blocks.push_back(n);
}

void HCodeGen::set_include_site (unsigned n){
  include_insert_site = n;
}
void HCodeGen::set_cuda_site (unsigned n){
  cuda_kernel_site = n;
}

void HCodeGen::set_length_var(std::string name) {
  length_var_name.push_back(name);
}
void HCodeGen::set_loop_var(std::string name) {
  loop_var.push_back(name);
}
void HCodeGen::set_start_index(std::string name) {
  start_index_str.push_back(name);
}
void HCodeGen::set_arr_chain(Array_rw_chain arr_rw_chain){
   arr_chain.push_back(arr_rw_chain);
}
void HCodeGen::add_mem_xfer(struct hmem_xfer m,unsigned i) {
  if(mem_bufs_chain.size()<=i){
      std::vector<hmem_xfer> mem_bufs;
      mem_bufs_chain.push_back(mem_bufs);
  }
  if(pre_xfers_chain.size()<=i){
      std::vector<hmem_xfer> pre_bufs;
      pre_xfers_chain.push_back(pre_bufs);
  }
  if(h2d_xfers_chain.size()<=i){
      std::vector<hmem_xfer> h2d_bufs;
      h2d_xfers_chain.push_back(h2d_bufs);
  }
  if(post_xfers_chain.size()<=i){
      std::vector<hmem_xfer> post_bufs;
      post_xfers_chain.push_back(post_bufs);
  }
  if(d2h_xfers_chain.size()<=i){
      std::vector<hmem_xfer> d2h_bufs;
      d2h_xfers_chain.push_back(d2h_bufs);
  }
  mem_bufs_chain[i].push_back (m);

  if (m.type & 1)
    pre_xfers_chain[i].push_back (m);
  if (m.type & 2)
    h2d_xfers_chain[i].push_back (m);

  if (m.type & 8)
    post_xfers_chain[i].push_back(m);
  if (m.type & 4)
    d2h_xfers_chain[i].push_back (m);
}
void HCodeGen::generateOCLDevFile() {
  if(!enter_loop.empty())
    if (enter_loop.front() <= 0)
  return;
  if(enter_loop.empty())
      return;
  fstream Infile(InputFile.c_str());
  if (!Infile) {
      cerr << "\nError. File " << InputFile << " has not found.\n";
      return;
  }
  std::string Line = std::string();
  ofstream File("kernel.cl");
  cerr << "\nWriting output to dev file kernel.cl\n";

  unsigned LineNo = 1;
  unsigned loop_id = 0;
  int kernel_num = 0;
  while (!Infile.eof() && loop_id < enter_loop.size()) {
      Line = std::string();
      std::getline(Infile, Line);
      if (replace_line[loop_id] == LineNo) {
          std::string::size_type start_i = Line.find("for");
          if (start_i != std::string::npos) {
              std::string::size_type init_begin, init_end, cond_begin, cond_end;
              init_begin = Line.find("=", start_i) + 1;
              init_end = Line.find(";", init_begin);
              cond_begin = Line.find("<", init_end) + 1;
              cond_end = Line.find(";", cond_begin);

              Line.replace(cond_begin, cond_end - cond_begin, " end_index");
              Line.replace(init_begin, init_end - init_begin, " start_index");
              //      loop_var = Line.substr(init_end + 1, cond_begin - init_end - 2);
          }

          File << "\n";
      }
      else if (in_loop(LineNo, loop_id)) {
          File << Line << "\n";
      }
      if (enter_loop[loop_id] == LineNo) {

          //Add arguments decl.
          std::string parameters_str;
          for (unsigned i = 0; i < arg_decls_chain[loop_id].size(); i++) {
              if (i > 0)
                  parameters_str += ",\n\t";
              std::string decl_str = arg_decls_chain[loop_id][i].type_name;
              std::string::size_type idx = decl_str.find("*");
              if (idx == std::string::npos) {
                  decl_str += " ";
                  decl_str += arg_decls_chain[loop_id][i].var_name;
              }
              else {
                  //Handle for mult-dim array, its decl should be TYPE (*name)[nn]
                  decl_str.insert(idx + 1, arg_decls_chain[loop_id][i].var_name);
                  parameters_str += "__global ";
              }
              std::string::size_type t_idx = decl_str.find("const");
              if (t_idx != std::string::npos)
                  decl_str.erase(t_idx, 5);
              parameters_str += decl_str;
          }

          //Add kernel function decl.
          File << "__kernel void my_kernel"
              << kernel_num
              << "\n"
              << " ( " << parameters_str;
          File << ")\n{\n\n";
          kernel_num++;
          //Add local variables decl.
          int is_def = 0;
          for (unsigned i = 0; i < var_decls_chain[loop_id].size(); i++) {
              File << "  " << var_decls_chain[loop_id][i].type_name
                  << " " << var_decls_chain[loop_id][i].var_name << ";\n";
              if (var_decls_chain[loop_id][i].var_name == loop_var[loop_id])
                  is_def = 1;
          }

          if (is_def == 0)
              File << "  int " << loop_var[loop_id] << " = get_global_id(0);\n";
          else
              File << loop_var[loop_id] << " = get_global_id(0);\n";
      }
      if (exit_loop[loop_id] == LineNo) {
          File << Line << "\n}";
          loop_id++;
      }
      LineNo++;
  }
  File.close();
}

void HCodeGen::generateOCLHostFile() {
  std::string global_work_size,global_work_size_f,global_work_size_e,block_size,min_,max_,global_dim,_fdiv;
  std::string global_offset_f,global_offset_m,global_offset_e;

  if(enter_loop.empty())
      return;
  unsigned loop_id=0;
  unsigned loop_id_max=enter_loop.size();
  std::string start_space;
  if (enter_loop[loop_id] <= 0)
  return;

  fstream Infile(InputFile.c_str());
  if (!Infile){
    cerr << "\nError. File " << InputFile << " has not found.\n";
    return;
  }
  std::string Line = std::string();
  ofstream File(HostFile.c_str());
  cerr << "\nWriting output to host file " << HostFile<< "\n";

  //Add hStreams include file;
  File << "#include \"set_env.h\"\n";

  unsigned LineNo = 1;

  while (!Infile.eof()) {
    Line = std::string();
    std::getline(Infile, Line);

  if (init_site[loop_id] <= LineNo && init_site_e[loop_id]>= LineNo) {
/*
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t'|| *It == '\n') {
	Start += *It;
      }
      else
          break;
    }
*/
  }
  if((init_site_e[loop_id]+1) == LineNo){
      std::string Start;
      for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
           ++ It) {
          if (*It == ' ' || *It == '\t'|| *It == '\n') {
              Start += *It;
          }
          else
              break;
      }
      Start += '  \n';
      unsigned kernel_id;
      File <<Start<<"read_cl_file();"
           <<Start<<"cl_initialization();"
           <<Start<<"cl_load_prog();\n"
           <<Start<<"printf(\"\%d\\t\%d\\t\%d\\t\\n\", " <<length_var_name[loop_id]<<", 1, tasks);\n";
      File << Start << "size_t localThreads[1] = {8};\n";

  }

  if (create_mem_site[0] == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    //Add hStreams buf create code
    for (unsigned loop_num=0;loop_num<loop_id_max;loop_num++){
        for (unsigned i = 0; i < mem_bufs_chain[loop_num].size(); i++) {
            hmem_xfer buffer=mem_bufs_chain[loop_num][i];
            unsigned num=0;
            for(auto c_buffer:created_xfers_chain)
            {
                if(buffer.buf_name==c_buffer.buf_name)
                    num=1;
            }
            if(num==0){
                created_xfers_chain.push_back(buffer);
                File <<Start<<"cl_mem "<<buffer.buf_name<<"_mem_obj = "
                     <<"clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, "
                     <<buffer.size_string<<", NULL, NULL);\n";
            }
        }
    }

  }
  if (enter_loop[loop_id] == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    for(unsigned pre_id=0;pre_id<=loop_id;pre_id++)
        for (unsigned i = 0;i < pre_xfers_chain[pre_id].size(); i++) {
            hmem_xfer buffer = pre_xfers_chain[pre_id][i];
            for(int id=0;id<arr_chain.size();id++) {
                if (arr_chain[id].name == buffer.buf_name)
                    if (arr_chain[id].start_place == enter_loop[loop_id]) {
                        //writed_xfers_chain.push_back(buffer);
                        File << Start << "errcode = clEnqueueWriteBuffer(clCommandQue[0], "
                             << buffer.buf_name << "_mem_obj, CL_TRUE, 0,\n"
                             << Start << buffer.size_string << ", \n"
                             << Start << buffer.buf_name << ", 0, NULL, NULL);\n";
                    }
            }
        }

      for (unsigned i = 0; i < fix_decls_chain[loop_id].size(); i++) {
        hvar_decl f_decl = fix_decls_chain[loop_id][i];
        writed_fix_decls.push_back(f_decl);
        std::string::size_type idx = f_decl.type_name.find("*");
        File <<Start<<"clSetKernelArg(clKernel"
             <<loop_id<<", "
             <<f_decl.id<<", ";
        if (idx != std::string::npos)
            File <<"sizeof(cl_mem), (void *) &"<<f_decl.var_name<<"_mem_obj);\n";
        else
            File <<"sizeof("<<f_decl.type_name<<"), &"<<f_decl.var_name<<");\n";
    }

    //Use idx_subtask is to avoid be the same to original variable name
    if(loop_id==0)
        File <<Start<<"DeltaT();\n";
    if (h2d_xfers_chain[loop_id].size() > 0)
        File << Start << "for (int i = 0; i < tasks; i++)\n"
        << Start << "{\n"
        << Start << "  size_t globalOffset[1] = {i*" << length_var_name[loop_id] << "/tasks+" << start_index_str[loop_id] << "};\n"
        << Start << "  size_t globalThreads[1] = {" << length_var_name[loop_id] << "/tasks" << "};\n";
    else
        File << Start << "  size_t globalOffset[1] = {" << length_var_name[loop_id] << "};\n"
        << Start << "  size_t globalThreads[1] = {" << length_var_name[loop_id] << "};\n";
  
    for(unsigned pre_id=0;pre_id<=loop_id;pre_id++)
        for (unsigned i = 0;i < h2d_xfers_chain[pre_id].size(); i++) {
            hmem_xfer buffer = h2d_xfers_chain[pre_id][i];
            for (int id = 0; id < arr_chain.size(); id++) {
                if (arr_chain[id].name == buffer.buf_name)
                    if(arr_chain[id].start_place==enter_loop[loop_id]){
                        File <<Start<<"  clEnqueueWriteBuffer(clCommandQue[i], "
                             <<buffer.buf_name<<"_mem_obj, CL_FALSE, i*"
                             <<buffer.size_string<<"/tasks, "
                             <<buffer.size_string<<"/tasks, "
                             <<"&"<<buffer.buf_name<<"[i*"<<length_var_name[pre_id]<<"/tasks]";
                        for (unsigned j = 1; j < buffer.dim; j++)
                            File <<"[0]";
                        File <<", 0, NULL, NULL);\n";
                    }
            }
        }
    if (h2d_xfers_chain[loop_id].size() > 0)
        File << Start << "  clEnqueueNDRangeKernel(clCommandQue[i], clKernel"
        << loop_id << ", 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);\n";
    else
        File << Start << "  clEnqueueNDRangeKernel(clCommandQue[0], clKernel"
        << loop_id << ", 1, NULL, globalThreads, localThreads, 0, NULL, NULL);\n";
    for(unsigned pre_id=0;pre_id<=loop_id;pre_id++)
        for(unsigned i = 0;i < d2h_xfers_chain[pre_id].size(); i++){
            hmem_xfer buffer=d2h_xfers_chain[pre_id][i];
            for (int id = 0; id < arr_chain.size(); id++) {
                if (arr_chain[id].name == buffer.buf_name)
                    if (arr_chain[id].end_place == exit_loop[loop_id]) {
                        File <<Start<<"  clEnqueueReadBuffer(clCommandQue[i], "
                             <<buffer.buf_name<<"_mem_obj, CL_FALSE, i*"
                             <<buffer.size_string<<"/tasks, "
                             <<buffer.size_string<<"/tasks, "
                             <<"&"<<buffer.buf_name<<"[i*"<<length_var_name[pre_id]<<"/tasks]";
                        for (unsigned j = 1; j < buffer.dim; j++)
                            File <<"[0]";
                        File <<", 0, NULL, NULL);\n";
                    }
            }
        }
  
    if(h2d_xfers_chain[loop_id].size()>0){
        File <<Start<<"}\n";
        File <<Start<<"for (int i = 0; i < tasks; i++)\n"
             <<Start<<"  clFinish(clCommandQue[i]);\n";
    }
    else
        File <<Start<<"  clFinish(clCommandQue[0]);\n";

      if(loop_id==(loop_id_max-1))
        File <<Start<<"printf(\"\%f\\n\", DeltaT());\n";
    start_space=Start;
    LineNo++;
    continue;
  }
  if (in_loop(LineNo,loop_id)) {
    LineNo++;
    continue;
  }
  if (finish_site[loop_id_max-1] == LineNo) {
    std::string Start;
    for (std::string::iterator It = Line.begin(), E = Line.end(); It != E;
	++ It) {
      if (*It == ' ' || *It == '\t') {
	Start += *It;
      }
      else
	break;
    }
    for (unsigned loop_num=0;loop_num<loop_id_max;loop_num++) {
        for (unsigned i = 0; i < mem_bufs_chain[loop_num].size(); i++){
            unsigned num=0;
            hmem_xfer buffer=mem_bufs_chain[loop_num][i];
            for(auto realese_buffer:realesed_xfers_chain)
                if(realese_buffer.buf_name==buffer.buf_name)
                    num=1;
            if(num==0){
                realesed_xfers_chain.push_back(buffer);
                File <<Start<<"clReleaseMemObject("<<buffer.buf_name<<"_mem_obj);\n";
            }
        }
    }
    File<<Start<<"cl_clean_up();\n";
  }
  if (exit_loop[loop_id] == LineNo) {
    for(unsigned pre_id=0;pre_id<=loop_id;pre_id++)
        for (unsigned i = 0;i < post_xfers_chain[pre_id].size(); i++){
            hmem_xfer buffer=post_xfers_chain[pre_id][i];
            for (int id = 0; id < arr_chain.size(); id++) {
                if (arr_chain[id].name == buffer.buf_name)
                    if (arr_chain[id].end_place == enter_loop2[loop_id]) {
                        File <<start_space<<"errcode = clEnqueueReadBuffer(clCommandQue[0], "
                             <<buffer.buf_name<<"_mem_obj, CL_TRUE, 0,\n"
                             <<start_space<<buffer.size_string<<", \n"
                             <<start_space<<buffer.buf_name<<", 0, NULL, NULL);\n";
                    }
            }
        }
  
    if (post_xfers_chain[loop_id].size() > 0)
      File<<start_space<<"clFinish(clCommandQue[0]);\n";
    LineNo++;
    if(loop_id<loop_id_max-1)
        loop_id++;
    continue;
  }

  File<<Line <<"\n";
  LineNo++;
  }

  File.close();
}

void HCodeGen::generateOCLHeadFile() {
  std::string input_file="set_env_model.h";
  std::string output_file="set_env.h";
  fstream Infile(input_file.c_str());
  if (!Infile){
      cerr << "\nError. File " << input_file << " has not found.\n";
      return;
    }
  std::string Line = std::string();
  ofstream File(output_file.c_str());
  cerr << "\nWriting output to host file " << output_file<< "\n";
  unsigned kernel_num=enter_loop.size();
  unsigned LineNo = 1;
  while (!Infile.eof()) {
      Line = std::string();
      std::getline(Infile, Line);
      if(LineNo==21){
          for(unsigned kernel_id=0;kernel_id<kernel_num;kernel_id++)
          {
              File<<"cl_kernel "<< "clKernel"<<kernel_id<<";\n";
          }
      }
      if(LineNo==102){
          for(unsigned kernel_id=0;kernel_id<kernel_num;kernel_id++)
          {
              File<<"\tclKernel"<< kernel_id<< " = clCreateKernel(clProgram,"
                                <<"\"my_kernel"
                                <<kernel_id
                                <<"\""
                                <<", &errcode);\n";
              File<<"\tif(errcode != CL_SUCCESS) printf(\"Error in creating kernel\\n\");\n";
          }
      }
      File<<Line<<"\n";
      LineNo++;
  }
}

//===-------------------------- HCodeGen.cpp --------------------------===//

