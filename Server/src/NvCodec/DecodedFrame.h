#pragma once
#include <vector>
#include <EncodedFrame.h>

class CodecMemory
{
public:
	CodecMemory(int _size);

	~CodecMemory();

	EncodedFrame& AllocMem();

	EncodedFrame& Peek();

	void PushMem();

	int GetMaxMem();
private:

	static const int NUM_FRAMES = 4;
	EncodedFrame m_frames[NUM_FRAMES];

	volatile int m_read = 0;
	volatile int m_write = 1;

	int m_maxMem = 0;

};