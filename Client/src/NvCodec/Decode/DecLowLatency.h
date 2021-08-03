/*
* 
*/

//This is the decoder, it uses the decoder wrapper and decoder.lib which uses the nvidia video codec

#pragma once

#include <CudaError.h>
#include <ThreadBase.h>

#include <atomic>
#include <glm/glm.hpp>
#include "DecWrapper.h"

class Client;
class NetDataProvider;

class DecLowLatency : public ThreadBase
{
public:

	DecLowLatency();
	~DecLowLatency();
	void InitDecoder(CUcontext _context, unsigned char _eye, Client* _client, const glm::ivec2& _resolutionBaseLevel);
	void SpecificProcess();


	CUdeviceptr GetDeviceFramePtr();

	//void RunProcess();

	int GetLastFrameID();
	void ArtificialIncreaseFrameID();
	void SetCurrentFrame();

	void FrameIDBeginIncrement();

	bool WaitCurrentFrameReady(int _frameLag, int _frameID);
	void SetResolutionLevel(unsigned char _level);

	unsigned char GetEye() { return m_eye; }

	int GetWidth() { return m_width; }
	int GetHeight() { return m_height; }

private:
	static void ThreadStart(DecLowLatency* _self);

	
	DecWrapper* m_decoder = nullptr;

	NetDataProvider* m_dataProvider = nullptr;

	static const int FRAME_QUEUE_SIZE = 3;

	CUdeviceptr m_deviceFrame[FRAME_QUEUE_SIZE] = { 0,0,0 };
	CUcontext m_cuContext = 0;

	int m_width = 0;
	int m_height = 0;

	volatile int m_frameIDbegin = -1;

	unsigned char m_resolutionLevel = 0;
	unsigned char m_eye = 0;

	Client* m_client = nullptr;

	bool m_newFrameReady = false;
	std::mutex m_runMutex;
	std::condition_variable m_newFrameCond;

	std::atomic<int> m_decodedFrameID = -1;
	int m_lastNonRenderedFrameID = -1;

	//Frame buffering can be enabled if m_dynamicQueueSize is larger than 1.
	int m_dynamicQueueSize = 1;
	int m_swapIndexRead = 0;
	int m_swapIndexWrite = 0;

	//Should be large enough in all cases, but can be increased if needed
	static const int MAX_VIDEO_FRAME_SIZE = 1024 * 1024;

	uint8_t m_videoMemory[MAX_VIDEO_FRAME_SIZE];
};

