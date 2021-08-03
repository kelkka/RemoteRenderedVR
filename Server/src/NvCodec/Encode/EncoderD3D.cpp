
#include <omp.h>
#include "EncoderD3D.h"
#include "../DecodedFrame.h"
//#include <exception>
#include "../../Utility/Timers.h"
#include <ResolutionSettings.h>
#include <StatCalc.h>
//#include <cuda_d3d11_interop.h>

#include <wrl.h>

std::mutex EncoderD3D::m_mutex;

using Microsoft::WRL::ComPtr;

#pragma comment(lib, "DXGI.lib")

EncoderD3D::EncoderD3D()
{

}

EncoderD3D::~EncoderD3D()
{
	printf("Encoder delete\n");

	DestroyWorker();
	delete m_encoder;
	delete m_memory;
}

void EncoderD3D::CudaLock()
{
	m_mutex.lock();
	//cuvidCtxLock(m_cudaLock, 0); TODO
}

void EncoderD3D::CudaUnlock()
{
	m_mutex.unlock();
	//cuvidCtxUnlock(m_cudaLock, 0); TODO
}

void EncoderD3D::InitEncoder(
	NetworkEngine* _net, 
	unsigned char _eye, 
	int _resolutionBaseLevel, 
	ID3D11Device* _device, 
	ID3D11DeviceContext* _deviceContext)
{
	m_deviceContext = _deviceContext;
	m_device = _device;
	//m_cudaGraphcisResource = new cudaGraphicsResource * [_numGraphicsResources];

	//for (int i = 0; i < _numGraphicsResources; i++)
	//{
	//	m_cudaGraphcisResource[i] = _cudaGraphicsResources[i];
	//}

	m_bitRate = (int)StatCalc::GetParameter("BITRATE");

	printf("Bitrate is %d\n", m_bitRate);




	//TODO: TEMP
	//glm::ivec2 res = ResolutionSettings::Get().Resolution(0);
	glm::ivec2 res = ResolutionSettings::Get().Resolution(_resolutionBaseLevel);

	m_resLevel = _resolutionBaseLevel;

	m_width = res.x;
	m_height = res.y;

	m_eye = _eye;

	m_encoder = new EncWrapper();
	m_encoder->CreateWithD3D(_device, m_width, m_height, m_bitRate, (int)StatCalc::GetParameter("H265") > 0);

	//NvEncoderInitParam encodeCLIOptions;

	//m_initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
	//m_encodeConfig = { NV_ENC_CONFIG_VER };

	//try
	//{
	//	m_encoder = new NvEncoderD3D11(m_device, m_width, m_height, eFormat, 0, false, false);
	//}
	//catch (NVENCException e) 
	//{
	//	printf("NvEnc NvEncoderCuda failed. Code=%d %s", e.getErrorCode(), e.what());
	//	return;
	//}

	////std::unique_ptr<RGBToNV12ConverterD3D11> pConverter;
	////if (bForceNv12)
	////{
	////	pConverter.reset(new RGBToNV12ConverterD3D11(pDevice.Get(), pContext.Get(), nWidth, nHeight));
	////}

	//m_initializeParams.encodeConfig = &m_encodeConfig;
	//m_encoder->CreateDefaultEncoderParams(&m_initializeParams, encodeCLIOptions.GetEncodeGUID(), encodeCLIOptions.GetPresetGUID(), NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY);

	//m_encodeConfig.gopLength = NVENC_INFINITE_GOPLENGTH;
	//m_encodeConfig.frameIntervalP = 1;
	//if (encodeCLIOptions.IsCodecH264())
	//{
	//	m_encodeConfig.encodeCodecConfig.h264Config.idrPeriod = NVENC_INFINITE_GOPLENGTH;
	//}
	//else
	//{
	//	m_encodeConfig.encodeCodecConfig.hevcConfig.idrPeriod = NVENC_INFINITE_GOPLENGTH;
	//}
	//m_encodeConfig.rcParams.lowDelayKeyFrameScale = 1;
	//m_encodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
	//m_encodeConfig.rcParams.multiPass = NV_ENC_TWO_PASS_QUARTER_RESOLUTION;
	//m_encodeConfig.rcParams.averageBitRate = m_bitRate;
	//m_encodeConfig.rcParams.vbvBufferSize =  (m_bitRate * m_initializeParams.frameRateDen / m_initializeParams.frameRateNum);
	//m_encodeConfig.rcParams.maxBitRate = m_bitRate;
	//m_encodeConfig.rcParams.vbvInitialDelay =  m_encodeConfig.rcParams.vbvBufferSize;
	//m_encodeConfig.rcParams.zeroReorderDelay = 1;

	//encodeCLIOptions.SetInitParams(&m_initializeParams, eFormat);

	//m_encoder->CreateEncoder(&m_initializeParams);

	//CUVIDD
	//NvEncGetEncodeCaps(m_encoder, );

	m_memory = new CodecMemory(m_width * m_height * 4);

	m_network = _net;


	m_worker = std::thread(ThreadStart, this);


}

void EncoderD3D::ThreadStart(EncoderD3D* _self)
{
	//ck(cuCtxSetCurrent((CUcontext)_self->m_encoder->GetDevice()));
	//uint32_t nBufSize = _self->m_width * _self->m_height * 4;
	//ck(cuMemAlloc(&_self->m_cudaMemory, nBufSize));
	//ck(cuCtxSetCurrent(0));

	_self->RunProcess();
}

void EncoderD3D::StartEncoding(long long _t_begin_render, long long _t_begin_encode, long long _t_input, ID3D11Texture2D* _texture)
{
	m_sceneTexture = _texture;

	m_t_begin_render = _t_begin_render;
	m_t_begin_encode = _t_begin_encode;
	m_t_input = _t_input;

	CopyTexture();

	//GoProcess();
}

void EncoderD3D::SpecificProcess()
{
	EncodedFrame& frame = m_memory->AllocMem();

	frame.TimeStamps[EncodedFrame::T_0_INPUT_SEND] = m_t_input;
	frame.TimeStamps[EncodedFrame::T_1_RENDER_START] = m_t_begin_render;
	frame.TimeStamps[EncodedFrame::T_2_ENCODER_START] = m_t_begin_encode;

	//ck(cuCtxSetCurrent(m_cuContext));
	//CudaLock();

	//CUDA_ERROR_CHECK
	//CudaLock();
	//CopyTexture();

	//cudaError_t err = cudaGraphicsMapResources(1, &m_cudaGraphcisResource[m_resLevel]);

	//void* devPtr = 0;
	//size_t availableSize = 0;
	//err = cudaGraphicsResourceGetMappedPointer(&devPtr, &availableSize, m_cudaGraphcisResource[m_resLevel]);

	//const glm::ivec2& res = ResolutionSettings::Get().Resolution(m_resLevel);
	//size_t requiredSize = res.x * res.y * 4;
	//if (availableSize < requiredSize || err != 0)
	//{
	//	printf("Cuda GL Size mismatch! av: %zu req: %zu Error: %d\n", availableSize, requiredSize, err);
	//	//CudaUnlock();
	//}

	//NvEncoderCuda::CopyToDeviceFrame(m_cuContext,
	//	devPtr,
	//	0,
	//	(CUdeviceptr)encoderInputFrame->inputPtr,
	//	(int)encoderInputFrame->pitch,
	//	m_encoder->GetEncodeWidth(),
	//	m_encoder->GetEncodeHeight(),
	//	CU_MEMORYTYPE_DEVICE,
	//	encoderInputFrame->bufferFormat,
	//	encoderInputFrame->chromaOffsets,
	//	encoderInputFrame->numChromaPlanes);

	//ck(cudaGraphicsUnmapResources(1, &m_cudaGraphcisResource[m_resLevel]));

	//NV_ENC_PIC_PARAMS picParams = {NV_ENC_PIC_PARAMS_VER};
	//picParams.encodePicFlags = 0;

	////if (m_forceIDR)
	////{
	////	printf("Force IDR\n");
	////	picParams.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR;
	////	m_forceIDR = false;
	////}

	//m_encoder->EncodeFrame(m_packets, &picParams);

	m_encoder->EncodeWithD3D(m_packets);

	//CudaUnlock();
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

	//CudaUnlock();

	//printf("Encode %lld			\n", m_t_encoded - m_t_render);

	CUDA_ERROR_CHECK

	//ck(cuCtxSetCurrent(0));
	
}

void EncoderD3D::CopyTexture()
{
	CudaLock();
	m_deviceContext->CopyResource(m_encoder->GetFrameD3D(), m_sceneTexture);
	CudaUnlock();
}
