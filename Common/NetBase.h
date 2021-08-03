#pragma once

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <io.h>

#include "Packets.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <atomic>

class NetBase
{

public:
	NetBase();
	~NetBase();

	virtual int InitNetwork();
	void Destroy();
	void Disconnect();
	int Connect();

	virtual int	MessageUDP(int _size, void* _data)=0;

	long long GetPing();
	int GetConnectionProblem();
	virtual int MessageTCP(int _size, void * _data, int _channel);
	bool IsConnected();
	virtual void Update() = 0;

	static const int PACKET_BUFFER_SIZE = 512;
	static const int FRAME_BUFFER_SIZE = 8;

	void SetSimulationLoss(int _val);

protected:

	void AddSystemMessage(const char * _str, int type);
	int InitWSA();
	int BindSockets();
	void CloseSockets();
	virtual int BindSocketUDP(SOCKET& _socket, sockaddr_in& _address, unsigned short _port, const char * _ip);
	virtual int BindSocketTCP(SOCKET& _socket, sockaddr_in& _address, unsigned short _port, const char * _ip);

	virtual void ListenUDP() = 0;

	static int ListenUDP_ThreadLoop(void * ptr);
	static int ListenTCP_ThreadLoop(void * ptr);

	int Recv(SOCKET _socket, void * _data, int _length);
	virtual void ListenTCP(int _channel) = 0;
	void StopThreads();
	void HandleRetransmissionTimer(bool _isRedundant, long long _originalTime);
	virtual void StartThreads() = 0;

	//Config values
	std::string		IP_ADRESS = "127.0.0.1";

	//Internet
	sockaddr_in		m_addressUDP;
	sockaddr_in		m_addressTCP;
	SOCKET			m_socketUDP=0;
	SOCKET			m_socketTCP[2] = { 0,0 };
	long long		m_ping = 0;
	unsigned int	m_lastPingMsg = 0;
	int				m_connectionProblem=0;

	volatile bool	m_quit = 0;
	volatile bool	m_isConnecting = 0;

	std::vector<std::thread*> m_threadList;

	int m_TCPListeners = 0;

	std::atomic<int> m_threadID = 0;

	static int m_lossChance;

};