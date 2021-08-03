#pragma once

#include "cuda.h"
#include <stdio.h>

// #define CUDA_ERROR_CHECK { \
// cudaError_t cudaErr = cudaPeekAtLastError(); \
// if (cudaErr != cudaSuccess)\
// 	printf("CUDA Error! %d at line %d in %s\n", cudaErr,__LINE__,__FUNCTION__);\
// }\

#ifndef CUDA_ERROR_CHECK
#define CUDA_ERROR_CHECK
#endif

inline bool CudaErrorCheck(CUresult _error, int _line, const char *_file)
{
	if (_error != CUDA_SUCCESS)
	{
		const char *errName = nullptr;
		cuGetErrorName(_error, &errName);

		printf("CUDA Error: %s at %d in %s\n", errName, _line, _file);
		return false;
	}
	return true;
}

#define CudaErr(_function) CudaErrorCheck(_function, __LINE__, __FILE__)
