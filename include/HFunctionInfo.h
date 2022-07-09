//------------------------------------------------------------------------------
// C code to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------

#ifndef HFUNCTIONINFO_H
#define HFUNCTIONINFO_H

#include "HLoopInfo.h"

//TODO:Construct the caller and callee graph.
class HFunctionInfo : public HLoopInfo{
  private:


  public:

  HFunctionInfo() {}
  ~HFunctionInfo() {}
  int last_return_line;
  void genFileInfo(HFile_Info & fi);
  void setLastReturnLine(int lineno) {last_return_line = lineno;}
};

#endif
