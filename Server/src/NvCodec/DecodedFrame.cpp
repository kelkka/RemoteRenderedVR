#pragma once
#include "DecodedFrame.h"


CodecMemory::CodecMemory(int _size)
{
	m_maxMem = _size;

	for (int i = 0; i < NUM_FRAMES; i++)
	{
		m_frames[i].Memory = (unsigned char*)malloc(_size);
		memset(m_frames[i].Memory, 0, _size);
		m_frames[i].Size = 0;
	}
}

CodecMemory::~CodecMemory()
{
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		free(m_frames[i].Memory);
	}
}

EncodedFrame & CodecMemory::AllocMem()
{
	return m_frames[m_write];
}

EncodedFrame& CodecMemory::Peek()
{
	return m_frames[m_read];
}

void CodecMemory::PushMem()
{
	m_write = (m_write + 1) % NUM_FRAMES;
	m_read = (m_read + 1 ) % NUM_FRAMES;
}

int CodecMemory::GetMaxMem()
{
	return m_maxMem;
}
