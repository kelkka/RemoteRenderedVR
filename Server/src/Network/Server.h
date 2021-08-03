#pragma once
#include "NetBase.h"
#include "../Core/VRInputs.h"
#include <StatCalc.h>
#include <map>

class Server : public NetBase
{
public:
	Server(VRInputs* _vrinput);
	~Server();
	int InitNetwork();
	bool ClientIsConnected();
	bool ForceNewRender();
	int MessageUDP(int _size, void * _data);
	void PushSendAgainBuffer(int _eye, int _frame, int _packet, Pkt::Sv_FramePart * _data);
	void AckSendAgainBuffer(int _eye, int _frame, int _packet);
	Pkt::Sv_FramePart* GetSendAgainBufferData(int _eye, int _frame, int _packet);
	int MessageTCP(int _size, void * _data, int _channel);

	void Update();
	bool CheckForDisconnect();
	void DisconnectClient();
	void RestartListen();
	bool IsLastFrameLost(int _eye);
	void OnFrameACK(int _eye);

	void EnsureRetransmissions(int _eye, unsigned int _timeStamp, long long _timerStart);

	void NackInput(unsigned int _timeStamp);

	void AckInput(unsigned int _inputTimeStamp, unsigned int _frameTimestamp);

	//int GetRedundantInputPkts();
	unsigned int GetLastAckedInput();

	int GetClientKeyPress();

	int GetExtraClientKeyPress();
	int GetInputLossThisFrameReset();

private:
	static bool Predicate0();
	static bool Predicate1();
	static int ListenTCP_Join_ThreadLoop(void* ptr);
	int ListenForJoinTCP();

	void ListenUDP();

	void ListenTCP(int _channel);

	void StartThreads();

	int BindSocketUDP(SOCKET& _socket, sockaddr_in& _address, unsigned short _port, const char * _ip);
	int BindSocketTCP(SOCKET& _socket, sockaddr_in& _address, unsigned short _port, const char * _ip);

	SOCKET		m_clientSocket[2] = { 0,0 };
	Pkt::Cl_Input m_matrixHMDPose[8];
	VRInputs* m_VRInput;

	std::thread m_ThreadListenUDP;
	std::thread m_ThreadListenTCP;
	std::thread m_ThreadListenTCP_join;

	bool m_clientConnected = false;

	sockaddr_in m_clientUDPAddr;

	volatile Pkt::Sv_FramePart m_sendAgainBuffer[2][FRAME_BUFFER_SIZE][PACKET_BUFFER_SIZE];

	unsigned int m_sendAgainMaxIndex[2] = { 0,0 };

	volatile bool m_lastFrameLost[2];
	volatile bool m_clientIsStuck[2] = { false,false };

	int m_redundantInputPackets = 0;
	StatCalc m_statsACK = StatCalc("FirstVideoPktACK");

	//unsigned int m_frameToBeACKed[2] = { 0,0 };
	volatile long long m_ACKedFrame[2] = { -1,-1 };

	unsigned int m_ACKedInput = 0;

	std::map<unsigned int, unsigned char> m_inputPktsReceived;

	std::ofstream m_inputGraph;

	int m_clientKeyPress = 0;
	int m_extrahackKeyPress = 0;

	unsigned int m_redundantInputPktsTotal = 0;

	std::atomic<int> m_inputLossThisFrame = 0;

	static bool m_newFrameReady[2];
	std::mutex m_runMutex[2];
	std::condition_variable m_newFrameCond[2];
};