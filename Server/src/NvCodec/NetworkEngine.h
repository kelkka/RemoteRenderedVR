#pragma once
#include "ThreadBase.h"

//class Client;
class Server;
class DecLowLatency;
class EncoderBase;

class NetworkEngine: public ThreadBase
{
public:
	NetworkEngine(Server * _server, EncoderBase* _encoder);
	~NetworkEngine();
	static void ThreadStart(NetworkEngine * _self);
	void SpecificProcess();
	bool IsConnectionComplete() { return m_remainingTCPFrames == 0; }

private:
	int m_framesOnStack = 0;
	EncoderBase* m_encoder = nullptr;
	//Client* m_client;
	Server* m_server;

	unsigned long long m_remainingTCPFrames = 0;
	unsigned char m_previousLevel = 0;
	//static const int INITIAL_TCP_LIMIT = 2000;

	int m_maxPktSize = 0;
	int m_maxPktCounter = 0;

	//unsigned int lastInputpkt = 0;
};

