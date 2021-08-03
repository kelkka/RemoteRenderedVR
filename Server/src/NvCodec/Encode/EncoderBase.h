#pragma once

#define _WINSOCKAPI_

#include <ThreadBase.h>
#include "../NetworkEngine.h"

#include "../DecodedFrame.h"
#include "EncWrapper.h"

struct cudaGraphicsResource;

class EncoderBase : public ThreadBase
{
public:

	CodecMemory* GetMemoryPtr();
	int GetCurrentFrameNr();
	void ChangeResolution(int _level);
	void Reconfigure();
	void ChangeBitrate(int _rate);
	void ForceIDR() { m_forceIDR = true; }
	int GetBitrate();
	virtual void CudaLock() = 0;
	virtual void CudaUnlock() = 0;
protected:

	bool m_forceIDR = false;
	int m_bitRate = 20000000;

	CodecMemory* m_memory = nullptr;

	int m_width = 0;
	int m_height = 0;

	
	unsigned char m_resLevel = 0;
	unsigned char m_eye = 0;

	int m_frameNumber = -1;

	EncWrapper* m_encoder;


	NetworkEngine* m_network;

	long long m_t_begin_render = 0;
	long long m_t_begin_encode = 0;
	long long m_t_encoded = 0;
	long long m_t_input = 0;
};