#pragma once

#include <ThreadBase.h>

#include <atomic>
#include <glm/glm.hpp>
#include "DecWrapper.h"
#include "DecBase.h"
#include <codec_api.h>

class Client;
class NetDataProvider;

class DecOpenH264 : public DecBase
{
public:

	DecOpenH264();
	~DecOpenH264();
	void InitDecoder(unsigned char _eye, Client* _client, const glm::ivec2& _resolutionBaseLevel);
	void SpecificProcess();


	unsigned char* GetRgbPtr();

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
	typedef struct Pixel
	{
		unsigned char R;
		unsigned char G;
		unsigned char B;
		unsigned char A;
	} Pixel;

	static void DecOpenH264::YUV420toRGB(unsigned char y, unsigned char u, unsigned char v, Pixel& _rgba);
	static void ThreadStart(DecOpenH264* _self);

	unsigned char* m_yuv[3] = { 0,0,0 }; //YUV for FRAME_QUEUE_SIZE frames


	static const int FRAME_QUEUE_SIZE = 6;
	Pixel* m_rgbData[FRAME_QUEUE_SIZE];

	ISVCDecoder *pSvcDecoder;
	SBufferInfo sDstBufInfo;
	//SParserBsInfo sDstParseInfo;

};

