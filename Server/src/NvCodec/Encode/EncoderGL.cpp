
#include "EncoderGL.h"
#include "../DecodedFrame.h"
#include <cuda_gl_interop.h> //GL.h
#include "../../Utility/Timers.h"
#include <ResolutionSettings.h>
#include <StatCalc.h>


EncoderGL::EncoderGL()
{

}

EncoderGL::~EncoderGL()
{
	printf("Encoder delete\n");

	DestroyWorker();
// 	m_encoder->DestroyEncoder();
if(m_encoder != nullptr)
 	delete m_encoder;

	CudaErr(cuvidCtxLockDestroy(m_cudaLock));
	if(m_memory != nullptr)
	delete m_memory;
}

void EncoderGL::CudaLock()
{
	cuvidCtxLock(m_cudaLock, 0);
}

void EncoderGL::CudaUnlock()
{
	cuvidCtxUnlock(m_cudaLock, 0);
}

int EncoderGL::InitEncoder(NetworkEngine* _net, CUcontext _context, unsigned char _eye, int _resolutionBaseLevel)
{
	m_bitRate = (int)StatCalc::GetParameter("BITRATE");

	printf("Bitrate is %d\n", m_bitRate);

	//TODO: TEMP
	//glm::ivec2 res = ResolutionSettings::Get().Resolution(0);
	glm::ivec2 res = ResolutionSettings::Get().Resolution(_resolutionBaseLevel);

	m_resLevel = _resolutionBaseLevel;

	m_width = res.x;
	m_height = res.y;

	m_eye = _eye;

	m_cuContext = _context;
	CudaErr(cuCtxSetCurrent(m_cuContext));

	CudaErr(cuvidCtxLockCreate(&m_cudaLock, m_cuContext));

	m_encoder = new EncWrapper();

	if (m_encoder->CreateWithGL(m_cuContext, m_width, m_height, m_bitRate, (int)StatCalc::GetParameter("H265") > 0) < 0)
		return -1;

	//CUVIDD
	//NvEncGetEncodeCaps(m_encoder, );

	m_memory = new CodecMemory(m_width*m_height * 4);

	m_network = _net;


	m_worker = std::thread(ThreadStart, this);

	return 0;
}

void EncoderGL::ThreadStart(EncoderGL* _self)
{
	/*CudaErr(cuCtxSetCurrent((CUcontext)_self->m_encoder->GetDevice()));
	uint32_t nBufSize = _self->m_width * _self->m_height * 4;
	CudaErr(cuMemAlloc(&_self->m_cudaMemory, nBufSize));*/
	//ck(cuCtxSetCurrent(0));

	_self->RunProcess();
}

void EncoderGL::CopyGLtoCuda(cudaGraphicsResource* _ptr, size_t _size, long long _t_begin_render, long long _t_begin_encode, long long _t_input)
{
	CUresult result = cuCtxSetCurrent(m_cuContext);
	if (result != CUDA_SUCCESS)
	{
		printf("Cuda cuCtxSetCurrent fail! %d\n", result);
		return;
	}

	cudaError_t err = cudaGraphicsMapResources(1, &_ptr);

	if (err != cudaSuccess)
	{
		printf("Cuda cudaGraphicsMapResources fail! %d\n", err);
	}

	void* devPtr = 0;
	size_t availableSize = 0;
	err = cudaGraphicsResourceGetMappedPointer(&devPtr, &availableSize, _ptr);

	if (availableSize < _size || err != 0)
	{
		printf("Cuda GL Size mismatch! av: %zu req: %zu Error: %d\n", availableSize, _size, err);
		//CudaUnlock();
	}
	else
	{
		m_encoder->CopyToCuda(devPtr, _size, _ptr);
		cudaGraphicsUnmapResources(1, (cudaGraphicsResource_t*)&_ptr);

		//CudaUnlock();
		m_t_begin_render = _t_begin_render;
		m_t_begin_encode = _t_begin_encode;
		m_t_input = _t_input;
#ifndef MEASURE_CUDA_MEMCPY
		GoProcess();
#endif
	}

	CudaErr(cuCtxSetCurrent(0));
	
}

void EncoderGL::SpecificProcess()
{
	EncodedFrame& frame = m_memory->AllocMem();

	frame.TimeStamps[EncodedFrame::T_0_INPUT_SEND] = m_t_input;
	frame.TimeStamps[EncodedFrame::T_1_RENDER_START] = m_t_begin_render;
	frame.TimeStamps[EncodedFrame::T_2_ENCODER_START] = m_t_begin_encode;

	CudaErr(cuCtxSetCurrent(m_cuContext));
	CudaLock();

	//const NvEncInputFrame* encoderInputFrame = m_encoder->GetNextInputFrame();

	//NvEncoderCuda::CopyToDeviceFrame(m_cuContext,
	//	(uint8_t *)m_cudaMemory,
	//	0,
	//	(CUdeviceptr)encoderInputFrame->inputPtr,
	//	(int)encoderInputFrame->pitch,
	//	m_encoder->GetEncodeWidth(),
	//	m_encoder->GetEncodeHeight(),
	//	CU_MEMORYTYPE_DEVICE,
	//	encoderInputFrame->bufferFormat,
	//	encoderInputFrame->chromaOffsets,
	//	encoderInputFrame->numChromaPlanes);

	//NV_ENC_PIC_PARAMS picParams = {NV_ENC_PIC_PARAMS_VER};
	//picParams.encodePicFlags = 0;

	////if (m_forceIDR)
	////{
	////	printf("Force IDR\n");
	////	picParams.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR;
	////	m_forceIDR = false;
	////}

	//m_encoder->EncodeFrame(m_packets, &picParams);

	m_encoder->EncodeWithGL(m_cuContext, m_packets);

	//m_encoder->Flush(m_packets);

	if (m_packets[0].size() > 0)
	{ 
		if (m_packets.size() > 1)
		{
			printf("%zd frames encoded!\n", m_packets[0].size());
		}
	
		if (m_packets[0].size() > m_memory->GetMaxMem())
		{
			printf("Encoded frame didnt fit in memory!!\n");
		}

		frame.Size = (int)m_packets[0].size();
		memcpy(frame.Memory, m_packets[0].data(), m_packets[0].size());
		frame.Index = ++m_frameNumber;
		frame.Level = m_resLevel;
		frame.Eye = m_eye;

		frame.TimeStamps[EncodedFrame::T_3_NETWORK_START] = Timers::Get().Time();

		m_memory->PushMem();

		//printf("Encoder starting network %d\n", frame.Index);
		m_network->GoProcess();
	}
	else
	{
		printf("No frame was encoded!\n");
	}

	CudaUnlock();

	//printf("Encode %lld			\n", m_t_encoded - m_t_render);

	CUDA_ERROR_CHECK

	//ck(cuCtxSetCurrent(0));
	
}
