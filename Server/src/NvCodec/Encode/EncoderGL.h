#pragma once

#define _WINSOCKAPI_

#include "../NetworkEngine.h"

//#include "../NvEncoderCuda.h"
//#include "../Utils/NvCodecUtils.h"
#include <cuviddec.h>
#include "../DecodedFrame.h"
#include "EncoderBase.h"

class EncoderGL : public EncoderBase
{
public:

	int InitEncoder(NetworkEngine* _net, CUcontext _context, unsigned char _eye, int _resolutionBaseLevel);
	//void CopyGLtoCuda(cudaGraphicsResource* _ptr, size_t _size);
	void CopyGLtoCuda(cudaGraphicsResource * _ptr, size_t _size, long long _t_begin, long long _t_render, long long _t_input);

	void SpecificProcess();

	EncoderGL();

	~EncoderGL();

	void CudaLock();
	void CudaUnlock();
private:

	static void ThreadStart(EncoderGL* _self);

	std::vector<std::vector<uint8_t>> m_packets;

	CUcontext m_cuContext = 0;
	CUvideoctxlock m_cudaLock;
};