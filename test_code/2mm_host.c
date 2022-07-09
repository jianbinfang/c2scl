#include "set_env.h"
/**
 * 2mm.c: This file is part of the PolyBench 3.0 test suite.
 *
 *
 * Contact: Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://polybench.sourceforge.net
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include "polybench.h"

/* Include benchmark-specific header. */
/* Default data type is double, default size is 4000. */
#include "2mm.h"


/* Array initialization. */
static
void init_array(int ni, int nj, int nk, int nl,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D(A,NI,NK,ni,nl),
		DATA_TYPE POLYBENCH_2D(B,NK,NJ,nk,nj),
		DATA_TYPE POLYBENCH_2D(C,NL,NJ,nl,nj),
		DATA_TYPE POLYBENCH_2D(D,NI,NL,ni,nl))
{
  int i, j;

  *alpha = 32412;
  *beta = 2123;
  for (i = 0; i < ni; i++)
    for (j = 0; j < nk; j++)
      A[i][j] = ((DATA_TYPE) i*j) / ni;
  for (i = 0; i < nk; i++)
    for (j = 0; j < nj; j++)
      B[i][j] = ((DATA_TYPE) i*(j+1)) / nj;
  for (i = 0; i < nl; i++)
    for (j = 0; j < nj; j++)
      C[i][j] = ((DATA_TYPE) i*(j+3)) / nl;
  for (i = 0; i < ni; i++)
    for (j = 0; j < nl; j++)
      D[i][j] = ((DATA_TYPE) i*(j+2)) / nk;
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int ni, int nl,
		 DATA_TYPE POLYBENCH_2D(D,NI,NL,ni,nl))
{
  int i, j;

  for (i = 0; i < ni; i++)
    for (j = 0; j < nl; j++) {
	fprintf (stderr, DATA_PRINTF_MODIFIER, D[i][j]);
	if ((i * ni + j) % 20 == 0) fprintf (stderr, "\n");
    }
  fprintf (stderr, "\n");
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_2mm(int ni, int nj, int nk, int nl,
		DATA_TYPE alpha,
		DATA_TYPE beta,
		DATA_TYPE POLYBENCH_2D(tmp,NI,NJ,ni,nj),
		DATA_TYPE POLYBENCH_2D(A,NI,NK,ni,nk),
		DATA_TYPE POLYBENCH_2D(B,NK,NJ,nk,nj),
		DATA_TYPE POLYBENCH_2D(C,NL,NJ,nl,nj),
		DATA_TYPE POLYBENCH_2D(D,NI,NL,ni,nl))
{
 
read_cl_file(); 
cl_initialization(); 
cl_load_prog();
 
printf("%d\t%d\t%d\t\n", (((ni)-1)-0+1), 1, tasks);
 
size_t localThreads[1] = {8};
 

#pragma scop
  /* D := alpha*A*B*C + beta*D */
  cl_mem tmp_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((nj)-1)+1)*sizeof(double [1024]), NULL, NULL);
  cl_mem A_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((nk)-1)+1)*sizeof(double [1024]), NULL, NULL);
  cl_mem B_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((nj)-1)+1)*sizeof(double [1024]), NULL, NULL);
  cl_mem D_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((nl)-1)+1)*sizeof(double [1024]), NULL, NULL);
  cl_mem C_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((nl)-1)+1)*sizeof(double [1024]), NULL, NULL);
  errcode = clEnqueueWriteBuffer(clCommandQue[0], B_mem_obj, CL_TRUE, 0,
  (((nj)-1)+1)*sizeof(double [1024]), 
  B, 0, NULL, NULL);
  clSetKernelArg(clKernel0, 0, sizeof(int), &ni);
  clSetKernelArg(clKernel0, 1, sizeof(int), &nj);
  clSetKernelArg(clKernel0, 2, sizeof(int), &nk);
  clSetKernelArg(clKernel0, 3, sizeof(double), &alpha);
  clSetKernelArg(clKernel0, 4, sizeof(cl_mem), (void *) &tmp_mem_obj);
  clSetKernelArg(clKernel0, 5, sizeof(cl_mem), (void *) &A_mem_obj);
  clSetKernelArg(clKernel0, 6, sizeof(cl_mem), (void *) &B_mem_obj);
  DeltaT();
  for (int i = 0; i < tasks; i++)
  {
    size_t globalOffset[1] = {i*(((ni)-1)-0+1)/tasks+0};
    size_t globalThreads[1] = {(((ni)-1)-0+1)/tasks};
    clEnqueueWriteBuffer(clCommandQue[i], tmp_mem_obj, CL_FALSE, i*(((nj)-1)+1)*sizeof(double [1024])/tasks, (((nj)-1)+1)*sizeof(double [1024])/tasks, &tmp[i*(((ni)-1)-0+1)/tasks][0], 0, NULL, NULL);
    clEnqueueWriteBuffer(clCommandQue[i], A_mem_obj, CL_FALSE, i*(((nk)-1)+1)*sizeof(double [1024])/tasks, (((nk)-1)+1)*sizeof(double [1024])/tasks, &A[i*(((ni)-1)-0+1)/tasks][0], 0, NULL, NULL);
    clEnqueueNDRangeKernel(clCommandQue[i], clKernel0, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);
  }
  for (int i = 0; i < tasks; i++)
    clFinish(clCommandQue[i]);
  errcode = clEnqueueWriteBuffer(clCommandQue[0], C_mem_obj, CL_TRUE, 0,
  (((nl)-1)+1)*sizeof(double [1024]), 
  C, 0, NULL, NULL);
  clSetKernelArg(clKernel1, 0, sizeof(int), &ni);
  clSetKernelArg(clKernel1, 1, sizeof(int), &nl);
  clSetKernelArg(clKernel1, 2, sizeof(double), &beta);
  clSetKernelArg(clKernel1, 3, sizeof(int), &nj);
  clSetKernelArg(clKernel1, 4, sizeof(cl_mem), (void *) &D_mem_obj);
  clSetKernelArg(clKernel1, 5, sizeof(cl_mem), (void *) &tmp_mem_obj);
  clSetKernelArg(clKernel1, 6, sizeof(cl_mem), (void *) &C_mem_obj);
  for (int i = 0; i < tasks; i++)
  {
    size_t globalOffset[1] = {i*(((ni)-1)-0+1)/tasks+0};
    size_t globalThreads[1] = {(((ni)-1)-0+1)/tasks};
    clEnqueueWriteBuffer(clCommandQue[i], D_mem_obj, CL_FALSE, i*(((nl)-1)+1)*sizeof(double [1024])/tasks, (((nl)-1)+1)*sizeof(double [1024])/tasks, &D[i*(((ni)-1)-0+1)/tasks][0], 0, NULL, NULL);
    clEnqueueNDRangeKernel(clCommandQue[i], clKernel1, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);
    clEnqueueReadBuffer(clCommandQue[i], tmp_mem_obj, CL_FALSE, i*(((nj)-1)+1)*sizeof(double [1024])/tasks, (((nj)-1)+1)*sizeof(double [1024])/tasks, &tmp[i*(((ni)-1)-0+1)/tasks][0], 0, NULL, NULL);
    clEnqueueReadBuffer(clCommandQue[i], D_mem_obj, CL_FALSE, i*(((nl)-1)+1)*sizeof(double [1024])/tasks, (((nl)-1)+1)*sizeof(double [1024])/tasks, &D[i*(((ni)-1)-0+1)/tasks][0], 0, NULL, NULL);
  }
  for (int i = 0; i < tasks; i++)
    clFinish(clCommandQue[i]);
  printf("%f\n", DeltaT());
#pragma endscop

clReleaseMemObject(tmp_mem_obj);
clReleaseMemObject(A_mem_obj);
clReleaseMemObject(B_mem_obj);
clReleaseMemObject(D_mem_obj);
clReleaseMemObject(C_mem_obj);
cl_clean_up();
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int ni = NI;
  int nj = NJ;
  int nk = NK;
  int nl = NL;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  POLYBENCH_2D_ARRAY_DECL(tmp,DATA_TYPE,NI,NJ,ni,nj);
  POLYBENCH_2D_ARRAY_DECL(A,DATA_TYPE,NI,NK,ni,nk);
  POLYBENCH_2D_ARRAY_DECL(B,DATA_TYPE,NK,NJ,nk,nj);
  POLYBENCH_2D_ARRAY_DECL(C,DATA_TYPE,NL,NJ,nl,nj);
  POLYBENCH_2D_ARRAY_DECL(D,DATA_TYPE,NI,NL,ni,nl);

  /* Initialize array(s). */
  init_array (ni, nj, nk, nl, &alpha, &beta,
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(B),
	      POLYBENCH_ARRAY(C),
	      POLYBENCH_ARRAY(D));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_2mm (ni, nj, nk, nl,
	      alpha, beta,
	      POLYBENCH_ARRAY(tmp),
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(B),
	      POLYBENCH_ARRAY(C),
	      POLYBENCH_ARRAY(D));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(ni, nl,  POLYBENCH_ARRAY(D)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(tmp);
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(C);
  POLYBENCH_FREE_ARRAY(D);

  return 0;
}

