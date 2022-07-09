#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
//#include<iostream>
#include <sys/time.h>
#define DECOMP

#define MAX_SOURCE_SIZE (0x100000)
#define DIM_LOCAL_WORK_GROUP_X 32

char str_temp[1024];

cl_platform_id platform_id;
cl_device_id device_id;   
cl_uint num_devices;
cl_uint num_platforms;
cl_int errcode;
cl_context clGPUContext;
cl_command_queue *clCommandQue;
cl_program clProgram;
cl_kernel clKernel0;
cl_kernel clKernel1;

FILE *fp;
char *source_str;
size_t source_size;
int tasks = 1;

void read_cl_file()
{
	// Load the kernel source code into the array source_str
	fp = fopen("kernel.cl", "r");
	if (!fp) {
		fprintf(stdout, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose( fp );
}

void cl_initialization()
{
	
	// Get platform and device information
	errcode = clGetPlatformIDs(1, &platform_id, &num_platforms);
	if(errcode == CL_SUCCESS) printf("number of platforms is %d\n",num_platforms);
	else printf("Error getting platform IDs\n");

	errcode = clGetPlatformInfo(platform_id,CL_PLATFORM_NAME, sizeof(str_temp), str_temp,NULL);
	if(errcode == CL_SUCCESS) printf("platform name is %s\n",str_temp);
	else printf("Error getting platform name\n");

	errcode = clGetPlatformInfo(platform_id, CL_PLATFORM_VERSION, sizeof(str_temp), str_temp,NULL);
	if(errcode == CL_SUCCESS) printf("platform version is %s\n",str_temp);
	else printf("Error getting platform version\n");

	errcode = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &num_devices);
	if(errcode == CL_SUCCESS) printf("number of devices is %d\n", num_devices);
	else printf("Error getting device IDs\n");

	errcode = clGetDeviceInfo(device_id,CL_DEVICE_NAME, sizeof(str_temp), str_temp,NULL);
	if(errcode == CL_SUCCESS) printf("device name is %s\n",str_temp);
	else printf("Error getting device name\n");
	
	// Create an OpenCL context
	clGPUContext = clCreateContext( NULL, 1, &device_id, NULL, NULL, &errcode);
	if(errcode != CL_SUCCESS) printf("Error in creating context\n");
 
	//Create a command-queue
	clCommandQue = (cl_command_queue *) malloc(tasks * sizeof(cl_command_queue));
	for (int i = 0; i < tasks; i++)
	  clCommandQue[i] = clCreateCommandQueue(clGPUContext, device_id, 0, &errcode);
	if(errcode != CL_SUCCESS) printf("Error in creating command queue\n");
}


void cl_load_prog()
{
	// Create a program from the kernel source
	cl_program clProgram = clCreateProgramWithSource(
        clGPUContext,
        1,
        (const char **)&source_str,
        (const size_t *)&source_size,
        NULL);
//	clProgram = clCreateProgramWithSource(clGPUContext, 1, (const char **)&source_str, (const size_t *)&source_size, &errcode);

	if(errcode != CL_SUCCESS) printf("Error in creating program\n");

	// Build the program
	errcode = clBuildProgram(clProgram, 1, &device_id, NULL, NULL, NULL);
	if(errcode != CL_SUCCESS){ 
		printf("Error in building program\n");
		size_t len;
        char buffer[8 * 1024];
        
        printf("Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(clProgram, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
	}
			
	// Create the OpenCL kernel
	clKernel0 = clCreateKernel(clProgram,"my_kernel0", &errcode);
	if(errcode != CL_SUCCESS) printf("Error in creating kernel\n");
	clKernel1 = clCreateKernel(clProgram,"my_kernel1", &errcode);
	if(errcode != CL_SUCCESS) printf("Error in creating kernel\n");

	//clFinish(clCommandQue[0]);
}

void cl_clean_up()
{
	// Clean up
	for (int i = 0; i < tasks; i++)
	{
	  errcode = clFlush(clCommandQue[i]);
	  errcode = clFinish(clCommandQue[i]);
	}

	for (int i = 0; i < tasks; i++)
	  errcode = clReleaseCommandQueue(clCommandQue[i]);

	free(clCommandQue);
	if(errcode != CL_SUCCESS) printf("Error in cleanup\n");
}

double DeltaT() {
  double ret;
  static struct timeval _NewTime;
  static struct timeval _OldTime;

  gettimeofday(&_NewTime, NULL);
  ret = ((double)_NewTime.tv_sec + 1.0e-6 * (double)_NewTime.tv_usec) - ((double)_OldTime.tv_sec + 1.0e-6 * (double)_OldTime.tv_usec);

  _OldTime.tv_sec = _NewTime.tv_sec;
  _OldTime.tv_usec = _NewTime.tv_usec;

  return ret;
}


