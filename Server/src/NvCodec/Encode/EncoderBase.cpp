
#include "EncoderBase.h"
#include "../DecodedFrame.h"
#include <exception>
#include <ResolutionSettings.h>
#include <StatCalc.h>

//simplelogger::Logger* logger = simplelogger::LoggerFactory::CreateConsoleLogger();


int EncoderBase::GetCurrentFrameNr()
{
	return m_frameNumber;
}

CodecMemory* EncoderBase::GetMemoryPtr()
{
	return m_memory;
}

void EncoderBase::ChangeResolution(int _level)
{
	if (m_resLevel == _level)
		return;

	printf("Changing to level %d\n", _level);
	m_resLevel = _level;
	Reconfigure();
}

void EncoderBase::Reconfigure()
{
	CudaLock();

	glm::ivec2 res = ResolutionSettings::Get().Resolution(m_resLevel);
	m_encoder->Reconfigure(res.x, res.y, m_bitRate);

	CudaUnlock();
}

void EncoderBase::ChangeBitrate(int _rate)
{
	printf("Changing to bitrate %d\n", _rate);
	m_bitRate = _rate;
	Reconfigure();
}

int EncoderBase::GetBitrate()
{
	return m_bitRate;
}