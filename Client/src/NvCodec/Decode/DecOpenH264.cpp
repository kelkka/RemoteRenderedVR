/*
* 
*/

#include "DecOpenH264.h"

#include <omp.h>
#include "../../Utility/Timers.h"
#include "../../Network/Client.h"
#include "../NetworkDataProvider.h"
#include <stb_image_write.h>



/**
  * @page DecoderUsageExample
  *
  * @brief
  *   * An example for using the decoder for Decoding only or Parsing only
  *
  * Step 1:decoder declaration
  * @code
  *
  *  //decoder declaration
  *  ISVCDecoder *pSvcDecoder;
  *  //input: encoded bitstream start position; should include start code prefix
  *  unsigned char *pBuf =...;
  *  //input: encoded bit stream length; should include the size of start code prefix
  *  int iSize =...;
  *  //output: [0~2] for Y,U,V buffer for Decoding only
  *  unsigned char *pData[3] =...;
  *  //in-out: for Decoding only: declare and initialize the output buffer info, this should never co-exist with Parsing only
  *  SBufferInfo sDstBufInfo;
  *  memset(&sDstBufInfo, 0, sizeof(SBufferInfo));
  *  //in-out: for Parsing only: declare and initialize the output bitstream buffer info for parse only, this should never co-exist with Decoding only
  *  SParserBsInfo sDstParseInfo;
  *  memset(&sDstParseInfo, 0, sizeof(SParserBsInfo));
  *  sDstParseInfo.pDstBuff = new unsigned char[PARSE_SIZE]; //In Parsing only, allocate enough buffer to save transcoded bitstream for a frame
  *
  * @endcode
  *
  * Step 2:decoder creation
  * @code
  *  WelsCreateDecoder(&pSvcDecoder);
  * @endcode
  *
  * Step 3:declare required parameter, used to differentiate Decoding only and Parsing only
  * @code
  *  SDecodingParam sDecParam = {0};
  *  sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
  *  //for Parsing only, the assignment is mandatory
  *  sDecParam.bParseOnly = true;
  * @endcode
  *
  * Step 4:initialize the parameter and decoder context, allocate memory
  * @code
  *  pSvcDecoder->Initialize(&sDecParam);
  * @endcode
  *
  * Step 5:do actual decoding process in slice level;
  *        this can be done in a loop until data ends
  * @code
  *  //for Decoding only
  *  iRet = pSvcDecoder->DecodeFrameNoDelay(pBuf, iSize, pData, &sDstBufInfo);
  *  //or
  *  iRet = pSvcDecoder->DecodeFrame2(pBuf, iSize, pData, &sDstBufInfo);
  *  //for Parsing only
  *  iRet = pSvcDecoder->DecodeParser(pBuf, iSize, &sDstParseInfo);
  *  //decode failed
  *  If (iRet != 0){
  *      //error handling (RequestIDR or something like that)
  *  }
  *  //for Decoding only, pData can be used for render.
  *  if (sDstBufInfo.iBufferStatus==1){
  *      //output handling (pData[0], pData[1], pData[2])
  *  }
  * //for Parsing only, sDstParseInfo can be used for, e.g., HW decoding
  *  if (sDstBufInfo.iNalNum > 0){
  *      //Hardware decoding sDstParseInfo;
  *  }
  *  //no-delay decoding can be realized by directly calling DecodeFrameNoDelay(), which is the recommended usage.
  *  //no-delay decoding can also be realized by directly calling DecodeFrame2() again with NULL input, as in the following. In this case, decoder would immediately reconstruct the input data. This can also be used similarly for Parsing only. Consequent decoding error and output indication should also be considered as above.
  *  iRet = pSvcDecoder->DecodeFrame2(NULL, 0, pData, &sDstBufInfo);
  *  //judge iRet, sDstBufInfo.iBufferStatus ...
  * @endcode
  *
  * Step 6:uninitialize the decoder and memory free
  * @code
  *  pSvcDecoder->Uninitialize();
  * @endcode
  *
  * Step 7:destroy the decoder
  * @code
  *  DestroyDecoder(pSvcDecoder);
  * @endcode
  *
*/




DecOpenH264::DecOpenH264()
{
	
}

DecOpenH264::~DecOpenH264()
{
	DestroyWorker();

	if (m_dataProvider != nullptr)
		delete m_dataProvider;

	pSvcDecoder->Uninitialize();
	free(pSvcDecoder);

	for (int i = 0; i < FRAME_QUEUE_SIZE; i++)
	{
		free(m_rgbData[i]);
	}
}

void DecOpenH264::InitDecoder(unsigned char _eye, Client* _client, const glm::ivec2& res)
{
	m_width = res.x;
	m_height = res.y;

	//m_decoder = new NvDecoder(_context, m_width, m_height, true, codec, NULL, true, false,(const Rect *)0,(const Dim *)0, m_width, m_height);
	//m_decoder = new DecWrapper(_context, m_width, m_height, codec);

	m_client = _client;

	m_dataProvider = new NetDataProvider(_client, _eye);

	m_eye = _eye;

	m_worker = std::thread(ThreadStart, this);


	memset(&sDstBufInfo, 0, sizeof(SBufferInfo));

	//in-out: for Parsing only: declare and initialize the output bitstream buffer info for parse only, this should never co-exist with Decoding only
	//memset(&sDstParseInfo, 0, sizeof(SParserBsInfo));
	//sDstParseInfo.pDstBuff = new unsigned char[m_width * m_height * 4]; //In Parsing only, allocate enough buffer to save transcoded bitstream for a frame


	WelsCreateDecoder(&pSvcDecoder);

	//Step 3:declare required parameter, used to differentiate Decoding only and Parsing only

	SDecodingParam sDecParam = { 0 };
	sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
	//sDecParam.uiCpuLoad = 100;


	//sDecParam.bParseOnly = true;
	int val = 2;
	pSvcDecoder->SetOption(DECODER_OPTION_NUM_OF_THREADS, &val);

	pSvcDecoder->Initialize(&sDecParam);

	for(int i = 0; i < FRAME_QUEUE_SIZE; i++)
	{ 
		m_rgbData[i] = (Pixel*)malloc(m_width * m_height * sizeof(Pixel));
	}

	//m_yuv[0] = (unsigned char*)malloc(m_width * m_height);
	//m_yuv[1] = (unsigned char*)malloc(m_width * m_height);
	//m_yuv[2] = (unsigned char*)malloc(m_width * m_height);


	//*  //input: encoded bitstream start position; should include start code prefix
	//	*  unsigned char *pBuf = ...;
	//*  //input: encoded bit stream length; should include the size of start code prefix
	//	*  int iSize = ...;


	//*  //output: [0~2] for Y,U,V buffer for Decoding only
	//	*  unsigned char *pData[3] = ...;

	//in-out: for Decoding only: declare and initialize the output buffer info, this should never co-exist with Parsing only
	 

}

void DecOpenH264::ThreadStart(DecOpenH264* _self)
{
	//CudaErr(cuCtxSetCurrent(_self->m_cuContext));

	//for (int i = 0; i < _self->FRAME_QUEUE_SIZE; i++)
	//{
	//	CudaErr(cuMemAlloc(&_self->m_deviceFrame[i], _self->m_width * _self->m_height * 4));
	//	CudaErr(cuMemsetD8(_self->m_deviceFrame[i], 0, _self->m_width * _self->m_height * 4));
	//}



	_self->RunProcess();
}



int DecOpenH264::GetLastFrameID()
{
	return m_decodedFrameID;
}

void DecOpenH264::ArtificialIncreaseFrameID()
{
	m_decodedFrameID++;
}

void DecOpenH264::SetCurrentFrame()
{
	m_frameIDbegin = m_decodedFrameID;
}

void DecOpenH264::FrameIDBeginIncrement()
{
	m_frameIDbegin++;
}

//Outside main thread should call this and wait for decoder to have a frame ready, this will block until ready
bool DecOpenH264::WaitCurrentFrameReady(int _frameLag, int _frameID)
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

void DecOpenH264::SetResolutionLevel(unsigned char _level)
{
	if (m_resolutionLevel != _level)
	{
		m_resolutionLevel = _level;
	}
}

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) < (b)) ? (b) : (a))

void DecOpenH264::YUV420toRGB(unsigned char y, unsigned char u, unsigned char v, Pixel& _rgba)
{
	int r = (int)(y + 1.402f * (v - 128));
	int g = (int)(y - 0.344f * (u - 128) - 0.714f * (v - 128));
	int b = (int)(y + 1.772f * (u - 128));

	_rgba.B = glm::clamp(r, 0, 255);
	_rgba.G = glm::clamp(g, 0, 255);
	_rgba.R = glm::clamp(b, 0, 255);
	_rgba.A = 255;
}

//Decoding process
void DecOpenH264::SpecificProcess()
{
	//GetData will block until memory is available and copied from packets
	int nVideoBytes = m_dataProvider->GetData(m_videoMemory, MAX_VIDEO_FRAME_SIZE);

	if (nVideoBytes == 0)
	{
		m_quit = true;
		return;
	}

	memset(&sDstBufInfo, 0, sizeof(SBufferInfo));

	int count = 0;

	while(sDstBufInfo.iBufferStatus == 0)
	{ 
		int result = pSvcDecoder->DecodeFrameNoDelay(m_videoMemory, nVideoBytes, m_yuv, &sDstBufInfo);
		if (result != 0)
		{
			printf("DecodeFrameNoDelay failed!\n");
			break;
		}
		count++;
	}

	if(count > 1)
	printf("Big count %d\n", count);

	//for Decoding only, pData can be used for render.
	if (sDstBufInfo.iBufferStatus == 1) 
	{
		#define NUM_THREADS 0

		unsigned char* Y = m_yuv[0];
		unsigned char* U = m_yuv[1];
		unsigned char* V = m_yuv[2];
		Pixel* rgb = m_rgbData[m_swapIndexWrite];

#if NUM_THREADS > 0
#pragma omp parallel num_threads(NUM_THREADS)
		{
		int chunk = (sDstBufInfo.UsrData.sSystemBuffer.iHeight / NUM_THREADS);
		int yStart = omp_get_thread_num() * chunk;
		int YStop = yStart + chunk;

		for (int y = yStart; y < YStop; y++)
		{
			for (int x = 0; x < sDstBufInfo.UsrData.sSystemBuffer.iWidth; x++)
			{
				int srcPix = y * sDstBufInfo.UsrData.sSystemBuffer.iStride[0] + x;
				int dstPix = y * m_width + x;
				int uvPix = (y/2) * sDstBufInfo.UsrData.sSystemBuffer.iStride[1] + x / 2;

				unsigned char c_y = Y[srcPix];
				unsigned char c_u = U[uvPix];
				unsigned char c_v = V[uvPix];

				YUV420toRGB(c_y, c_u, c_v, rgb[dstPix]);
			}
		}
		}
#else
		for (int y = 0; y < sDstBufInfo.UsrData.sSystemBuffer.iHeight; y++)
		{
			for (int x = 0; x < sDstBufInfo.UsrData.sSystemBuffer.iWidth; x++)
			{
				int srcPix = y * sDstBufInfo.UsrData.sSystemBuffer.iStride[0] + x;
				int dstPix = y * m_width + x;
				int uvPix = (y / 2) * sDstBufInfo.UsrData.sSystemBuffer.iStride[1] + x / 2;

				unsigned char c_y = Y[srcPix];
				unsigned char c_u = U[uvPix];
				unsigned char c_v = V[uvPix];

				YUV420toRGB(c_y, c_u, c_v, rgb[dstPix]);
			}
	}

#endif
		//int result = stbi_write_png("hehehe.png", m_width, m_height, 4, m_rgbData[m_swapIndexWrite], m_width * 4);

		m_swapIndexWrite = (m_swapIndexWrite + 1) % m_dynamicQueueSize;

		m_decodedFrameID += 1;

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
	else
	{
		printf("sDstBufInfo.iBufferStatus was not 1!\n");
	}
}

unsigned char* DecOpenH264::GetRgbPtr()
{
	unsigned char* result = (unsigned char*)m_rgbData[m_swapIndexRead];
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