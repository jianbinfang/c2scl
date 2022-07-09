//------------------------------------------------------------------------------
// C code to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#include "HFunctionInfo.h"

void HFunctionInfo::genFileInfo(HFile_Info & fi) {
  fi.last_include = -1;
  fi.start_function = -1;
  fi.return_site = -1;
  fi.start_function_e = -1;

  //Find the file scope.
  HScopeIR *f_scope = this;
  while (f_scope && f_scope->get_type() != 1) f_scope = f_scope->parent;
  
  if (f_scope == NULL)
    return;

  //Find the first decl stmt in file scope.
  fi.last_include = f_scope->get_var(0)->get_pos()->get_line();
  fi.start_function = get_start_pos()->get_line();
  fi.start_function_e = get_pos()->get_line();
  fi.return_site = last_return_line;
}
