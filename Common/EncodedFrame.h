/*
* 
*/

#pragma once
#include <vector>

//Not really needed in the client, except for the enum TIMER_NAMES

struct EncodedFrame
{
	enum TIMER_NAMES
	{
		T_0_INPUT_SEND,
		T_1_RENDER_START,
		T_2_ENCODER_START,
		T_3_NETWORK_START,
	};

	unsigned char* Memory;
	int Size = 0;
	unsigned int Index = 0;
	unsigned char Level = 0;
	unsigned char Eye = 0;
	long long TimeStamps[4];
};