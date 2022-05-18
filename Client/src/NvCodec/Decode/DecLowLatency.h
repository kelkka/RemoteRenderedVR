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
#include "DecBase.h"

class Client;
class NetDataProvider;

class DecLowLatency : public DecBase
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
};

