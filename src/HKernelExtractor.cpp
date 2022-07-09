//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#include "HKernelExtractor.h"
bool HKernelExtractor::nouse(HLoopInfo * loop){
    if (loop->isParallelizable() == false)
        return false;

    build_kernel(loop);

    return true;
}
bool HKernelExtractor::visitloop(HLoopInfo * loop){
  if (loop->isParallelizable() == false)  
    return false;
 
  build_kernel(loop); 

  return true;
}

bool HKernelExtractor::build_kernel(HLoopInfo * loop){
  struct HKernel_Info k_info;
  
  k_info.replace_line = loop->get_line();
  k_info.enter_loop = loop->get_start_line();
  k_info.exit_loop = loop->get_end_line();

  HFile_Info fi_info;
  loop->get_func()->genFileInfo(fi_info);
  k_info.init_site = fi_info.start_function;
  k_info.init_site_e = fi_info.start_function_e;
  k_info.finish_site = fi_info.return_site;
  k_info.last_include = fi_info.last_include;

  struct hmem_xfer m;
  struct hvar_decl v;
  HPosition * decl_pos;
  HPosition * last_decl_pos;
  if(!loop->loopVars.empty())
      last_decl_pos=loop->loopVars.front()->get_pos();
  for(auto& out_var : loop->loopVars) {
    if(out_var->get_name()==loop->get_iter_name())
    {
        if(loop->get_start_value_str().empty())
            loop->set_start_value_str(out_var->avInloop[0]->value_range->get_lower_str());
        if(loop->iter_num.str.empty()){
            loop->iter_num.str = '(';
            loop->iter_num.str += out_var->avInloop[0]->value_range->get_upper_str();
            loop->iter_num.str += '-';
            loop->iter_num.str += out_var->avInloop[0]->value_range->get_lower_str();
            loop->iter_num.str += "+1";
            loop->iter_num.str += ')';
        }
    }
    v.type_name = out_var->get_type();
    v.var_name =  out_var->get_name();
    int category = out_var->get_category();
    if (category == 2 || category == 4) {
      v.type = 1;
    }
    else {
      v.type = 0;
    }
    if (out_var->isInductive())
	v.type += 2;

    if (v.type & 1) {
    	m.buf_name = v.var_name;
        HValueRange * av_in_loop1=out_var->avInloop[0]->get_index_value(0);
        m.size_string = '(';
    	m.size_string += av_in_loop1->get_upper_str();
    	m.size_string += "+1";
    	m.size_string += ')';
   	m.type_name = v.type_name;
   	m.elem_type = m.type_name;
   	m.elem_type = v.type_name;
      int idx = m.elem_type.find("(*)");
      if (idx >= 0) {
        m.elem_type.erase(idx, 3);
      }
      else if ((idx = m.elem_type.find("*")) >= 0) {
        m.elem_type.erase(idx, 1);
      }
      m.dim = out_var->get_dim();
      m.size_string += '*';
      m.size_string += "sizeof";
      m.size_string += '(';
      m.size_string += m.elem_type;
      m.size_string += ')';
      switch (out_var->access_type()) {
        case 3:
   	    m.type = 15;
          break;
        case 2:
   	    m.type = 14;
          break;
        case 1:
   	    m.type = 3;
          break;
        default: 
          m.type = 0;
          break;
      }
      if (out_var->canSplit() == true)
        m.type &= 6;
      else
        m.type &= 9;

      decl_pos = out_var->get_pos();
      if (last_decl_pos->get_line() < decl_pos->get_line())
        last_decl_pos = decl_pos;

      v.type=m.type;
      k_info.mem_bufs.push_back(m);
      k_info.pointer_parms.push_back(v);
    }
    else {
        if(out_var->avInloop[0]->value_range)
            k_info.local_parms.push_back(v);
        else
            k_info.val_parms.push_back(v);
    }
  }

  k_info.enter_loop2=k_info.exit_loop;
  k_info.length_var = loop->get_iter_num_str();
  k_info.loop_var = loop->get_iter_name();
  k_info.start_index = loop->get_start_value_str();
  k_info.insns = loop->get_insns_num();
  k_info.create_mem_site = last_decl_pos->get_line();
  int i_internal=1,ic_internal=1;
  int slip_flag=0;
  for(auto mem_buf_k:k_info.mem_bufs)
      if(mem_buf_k.type&2||mem_buf_k.type&4){
          slip_flag=1;
          break;
      }
  if(slip_flag==1){
      if(k_info_queue.empty())
          k_info_queue.push_back(k_info);
      for(auto k_info_internal:k_info_queue)
      {
          if(k_info.enter_loop>=k_info_internal.enter_loop&&k_info.exit_loop<=k_info_internal.exit_loop){
              i_internal=0;
              break;
          }

      }
      if(i_internal==1)
          k_info_queue.push_back(k_info);
  }
  else{
      if(k_info_queue_cant.empty())
          k_info_queue_cant.push_back(k_info);
      for(auto k_info_internal:k_info_queue_cant)
      {
          if(k_info.enter_loop>=k_info_internal.enter_loop&&k_info.exit_loop<=k_info_internal.exit_loop){
              ic_internal=0;
              break;
          }

      }
      if(ic_internal==1)
          k_info_queue_cant.push_back(k_info);
  }


  return true;
}
