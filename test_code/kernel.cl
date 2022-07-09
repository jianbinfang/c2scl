
__kernel void my_kernel0
 ( int ni,
	int nj,
	int nk,
	double alpha,
	__global double (*tmp)[1024],
	__global double (*A)[1024],
	__global double (*B)[1024])
{

  int i;
  int j;
  int k;
i = get_global_id(0);
    for (int j = 0; j < nj; j++)
      {
	tmp[i][j] = 0;
	for (int k = 0; k < nk; ++k)
	  tmp[i][j] += alpha * A[i][k] * B[k][j];
      }
}
__kernel void my_kernel1
 ( int ni,
	int nl,
	double beta,
	int nj,
	__global double (*D)[1024],
	__global double (*tmp)[1024],
	__global double (*C)[1024])
{

  int i;
  int j;
  int k;
i = get_global_id(0);
    for (int j = 0; j < nl; j++)
      {
	D[i][j] *= beta;
	for (int k = 0; k < nj; ++k)
	  D[i][j] += tmp[i][k] * C[k][j];
      }
}