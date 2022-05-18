#pragma once

#include <CudaError.h>
#include <ThreadBase.h>

#include <atomic>
#include <glm/glm.hpp>

class Client;
class NetDataProvider;

class DecBase : public ThreadBase
{
public:

	virtual int GetLastFrameID() = 0;
	virtual void ArtificialIncreaseFrameID() = 0;
	virtual void SetCurrentFrame() = 0;

	virtual void FrameIDBeginIncrement() = 0;

	virtual bool WaitCurrentFrameReady(int _frameLag, int _frameID) = 0;
	virtual void SetResolutionLevel(unsigned char _level) = 0;

	unsigned char GetEye() { return m_eye; }

	int GetWidth() { return m_width; }
	int GetHeight() { return m_height; }

protected:

	NetDataProvider* m_dataProvider = nullptr;
	int m_width = 0;
	int m_height = 0;
	unsigned char m_eye = 0;

	volatile int m_frameIDbegin = -1;

	unsigned char m_resolutionLevel = 0;

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
	static const int MAX_VIDEO_FRAME_SIZE = 1024 * 1024 * 16;
	uint8_t m_videoMemory[MAX_VIDEO_FRAME_SIZE];
};

