/*
* 
*/

//Provides data to decoder from network packets

#pragma once
#include <cuviddec.h>

class Client;

class NetDataProvider
{
public:
	NetDataProvider(Client* _client, int _eye);

	~NetDataProvider();

	// Fill in the buffer owned by the demuxer/decoder
	int GetData(uint8_t *pBuf, int nBuf);

private:

	Client* m_client = 0;

	int m_eye = 0;
};