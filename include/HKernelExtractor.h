//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#ifndef HKERNELEXTRACTOR_H
#define HKERNELEXTRACTOR_H

#include "HKernelInfo.h"
#include "HFunctionInfo.h"

class HKernelExtractor {
private:
  bool build_kernel(HLoopInfo *loop);

public:
  HKernelExtractor(std::vector<struct HKernel_Info> &k,std::vector<struct HKernel_Info> &t): k_info_queue(k),  k_info_queue_cant(t) {}

  bool nouse(HLoopInfo *loop);
  bool visitloop(HLoopInfo *loop);

private:
  std::vector<struct HKernel_Info> &k_info_queue;
  std::vector<struct HKernel_Info> &k_info_queue_cant;
};

#endif
