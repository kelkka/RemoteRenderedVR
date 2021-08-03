/*
* 
*/

#include "../Network/Client.h"
#include "NetworkDataProvider.h"

NetDataProvider::NetDataProvider(Client* _client, int _eye)
{
	m_client = _client;
	m_eye = _eye;
}

NetDataProvider::~NetDataProvider()
{
}

int NetDataProvider::GetData(uint8_t * pBuf, int nBuf)
{
	// Gets data from network
	while (!m_client->IsStopped())
	{
		int size = m_client->CopyFBBuffer(pBuf, m_eye, nBuf);

		//quit
		if (size == -2)
		{
			return 0;
		}

		if (size != -1)
		{
			return size;
		}

		printf("NetworkDataProvider decoder issue\n");
		//Packet loss
	}

	return 0;
}