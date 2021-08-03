/*
* 
*/

#include "DecLowLatency.h"

#include <omp.h>
#include "../../Utility/Timers.h"
#include "../../Network/Client.h"

#include "../NetworkDataProvider.h"
#include "nvcuvid.h"

DecLowLatency::DecLowLatency()
{
	
}

DecLowLatency::~DecLowLatency()
{
	DestroyWorker();

	if (m_decoder != nullptr)
		delete m_decoder;
	if (m_dataProvider != nullptr)
		delete m_dataProvider;

	for (int i = 0; i < FRAME_QUEUE_SIZE; i++)
	{
		CudaErr(cuMemFree(m_deviceFrame[i]));
	}
}

void DecLowLatency::InitDecoder(CUcontext _context, unsigned char _eye, Client* _client, const glm::ivec2& res)
{
	m_cuContext = _context;
	CudaErr(cuCtxSetCurrent(m_cuContext));

	m_width = res.x;
	m_height = res.y;

	cudaVideoCodec_enum codec = cudaVideoCodec_H264;

	if (StatCalc::GetParameter("H265") > 0)
		codec = cudaVideoCodec_HEVC;

	//m_decoder = new NvDecoder(_context, m_width, m_height, true, codec, NULL, true, false,(const Rect *)0,(const Dim *)0, m_width, m_height);
	m_decoder = new DecWrapper(_context, m_width, m_height, codec);

	m_client = _client;

	m_dataProvider = new NetDataProvider(_client, _eye);

	m_eye = _eye;

	CUDA_ERROR_CHECK
	m_worker = std::thread(ThreadStart, this);
}

void DecLowLatency::ThreadStart(DecLowLatency* _self)
{
	CudaErr(cuCtxSetCurrent(_self->m_cuContext));

	for (int i = 0; i < _self->FRAME_QUEUE_SIZE; i++)
	{
		CudaErr(cuMemAlloc(&_self->m_deviceFrame[i], _self->m_width * _self->m_height * 4));
		CudaErr(cuMemsetD8(_self->m_deviceFrame[i], 0, _self->m_width * _self->m_height * 4));
	}

	CUDA_ERROR_CHECK
	_self->RunProcess();
}



int DecLowLatency::GetLastFrameID()
{
	return m_decodedFrameID;
}

void DecLowLatency::ArtificialIncreaseFrameID()
{
	m_decodedFrameID++;
}

void DecLowLatency::SetCurrentFrame()
{
	m_frameIDbegin = m_decodedFrameID;
}

void DecLowLatency::FrameIDBeginIncrement()
{
	m_frameIDbegin++;
}

//Outside main thread should call this and wait for decoder to have a frame ready, this will block until ready
bool DecLowLatency::WaitCurrentFrameReady(int _frameLag, int _frameID)
{
	_frameLag = max(_frameLag, 0);
	m_dynamicQueueSize = (_frameLag + 1);

	//int decodedFrames = m_decodedFrameID - m_frameIDbegin;

	//long long t0 = Timers::Get().Time();

	//while (!m_quit && (((m_decodedFrameID + _frameLag) <= m_frameIDbegin) || decodedFrames == 0))
	while (!m_quit)
	{
		/*if ((m_decodedFrameID >= (m_frameIDbegin + _frameLag)) && decodedFrames > 0)
		{
			break;
		}*/

		if (m_decodedFrameID >= _frameID)
		{
			break;
		}

		{
			std::unique_lock<std::mutex> waitLock(m_runMutex);
			if (m_newFrameCond.wait_for(waitLock, std::chrono::microseconds(100), [this] { return m_newFrameReady; }))
			{
				m_newFrameReady = false;
			}
		}

		//decodedFrames = m_decodedFrameID - m_frameIDbegin;
	}

	//Timers::Get().PushLongLong(m_eye, T_JITTER, _frameID, Timers::Get().Time() - t0);	//sync wait

	

	//if (decodedFrames != 1)
	//	printf("%d frames decoded\n", decodedFrames);

	//printf("Decoder %d, from %d to %d\n", m_eye, m_frameIDbegin, m_decodedFrameID);

	return true;
}

void DecLowLatency::SetResolutionLevel(unsigned char _level)
{
	if (m_resolutionLevel != _level)
	{
		m_resolutionLevel = _level;
	}
}

//Decoding process
void DecLowLatency::SpecificProcess()
{
	CUresult result = cuCtxSetCurrent(m_cuContext);
	if (result != CUDA_SUCCESS)
	{
		printf("Cuda cuCtxSetCurrent fail! %d\n", result);
	}

	uint8_t **ppFrame;
	int nFrameReturned = 0;

	CUDA_ERROR_CHECK

	//GetData will block until memory is available and copied from packets
	int nVideoBytes = m_dataProvider->GetData(m_videoMemory, MAX_VIDEO_FRAME_SIZE);

	if (nVideoBytes == 0)
	{
		m_quit = true;
		return;
	}

	//Data received from network is here moved to the decoder
	//Set flag CUVID_PKT_ENDOFPICTURE to signal that a complete packet has been sent to decode
	nFrameReturned = m_decoder->Decode(m_videoMemory, nVideoBytes, &ppFrame);

	if (nFrameReturned > 0)
	{
		//Shouldn't happen
		if (nFrameReturned != 1)
		{
			printf("nFrameReturned %d!\n", nFrameReturned);
		}

		int nPitch = 4 * m_width;

		//printf("%s\n", m_decoder->GetVideoInfo().c_str());

		//Only really needed if frame buffering is wanted
		CUdeviceptr deviceFrame = m_deviceFrame[m_swapIndexWrite];
		m_swapIndexWrite = (m_swapIndexWrite + 1) % m_dynamicQueueSize;

		m_decoder->ColorConversion(ppFrame, deviceFrame, nPitch);

		CUDA_ERROR_CHECK

		m_decodedFrameID += nFrameReturned;

		int printId = m_decodedFrameID;

		if (m_client->IsMorePacketsToDecode(m_eye))
		{
			printId += 1;
			printf("More to decode...\n");
		}

		//Statistics
		Timers::Get().PushLongLong(m_eye, T_DEC_FIN, printId, Timers::Get().Time());

		//Notify render thread that a new frame is ready
		std::lock_guard<std::mutex> guard(m_runMutex);
		m_newFrameReady = true;
		m_newFrameCond.notify_one();
	}
}

CUdeviceptr DecLowLatency::GetDeviceFramePtr()
{
	CUdeviceptr result = m_deviceFrame[m_swapIndexRead];
	m_swapIndexRead = (m_swapIndexRead + 1) % m_dynamicQueueSize;
	return result;
}

//The decoder runs as fast as it can and doesnt wait for a sync-signal
//void DecLowLatency::RunProcess()
//{
//	while (m_quit == false)
//	{
//		SpecificProcess();
//	}
//}