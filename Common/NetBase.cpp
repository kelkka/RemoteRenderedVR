#include "NetBase.h"

#include <stdio.h>
#include <fstream>
#include <omp.h>
#include <intsafe.h>
#include <bitset>
#include <iostream>
#include <StatCalc.h>

int NetBase::m_lossChance = 0;

NetBase::NetBase()
{
	m_lossChance = (int)StatCalc::GetParameter("LOSS");
	IP_ADRESS = StatCalc::GetParameterStr("IP");
}

NetBase::~NetBase()
{
	//Disconnect();
}

bool NetBase::IsConnected()
{
	return (m_threadList.size() > 0 || m_isConnecting);
}

int NetBase::GetConnectionProblem()
{
	return m_connectionProblem;
}

int NetBase::MessageTCP(int _size, void* _data, int _channel)
{

	int sent = 0;

	while (sent < _size)
	{
		int err = send(m_socketTCP[_channel], (const char*)_data, _size, 0);
		if (err == -1)
		{
			int wsaerr = WSAGetLastError();
			printf("Failed to send TCP: %d\n", wsaerr);
			m_connectionProblem++;
			return wsaerr;
		}
		sent += err;
	}

	return 0;
}


int NetBase::ListenUDP_ThreadLoop(void* ptr)
{
	NetBase* self = (NetBase*)ptr;

	while (!self->m_quit)
	{
		self->ListenUDP();
	}

	return 1;
}


int NetBase::ListenTCP_ThreadLoop(void* ptr)
{
	NetBase* self = (NetBase*)ptr;

	int id = self->m_threadID.fetch_add(1);

	printf("Listening TCP on channel %d\n", id);

	while (!self->m_quit)
	{
		self->ListenTCP(id);
	}

	return 1;
}

int NetBase::Recv(SOCKET _socket, void* _data, int _length)
{
	int received = 0;
	char* data = (char*)_data;

	while (received < _length)
	{
		int recvNow = recv(_socket, data + received, _length - received, 0);
		received += recvNow;

		if (recvNow == SOCKET_ERROR)
			return SOCKET_ERROR;

		if (m_quit)
			return -1;
	}

	return received;
}


int NetBase::InitWSA()
{
	//Init WSA
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		printf("WSAStartup FAILED: %d\n", err);
		return -1;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		return -2;
	}
	else
		printf("Winsock 2.2 dll OK\n");

	return 0;
}

int NetBase::BindSockets()
{
	CloseSockets();

	StopThreads();

	if (BindSocketUDP(m_socketUDP, m_addressUDP, UDP_PORT, IP_ADRESS.c_str()) < 0)
		return -1;

	for (int i = 0; i < m_TCPListeners; i++)
	{
		if (BindSocketTCP(m_socketTCP[i], m_addressTCP, TCP_PORT, IP_ADRESS.c_str()) < 0)
			return -2;
	}

	return 0;
}

void NetBase::CloseSockets()
{
	m_connectionProblem = 0;

	//If reconnect
	if (m_socketUDP)
	{
		closesocket(m_socketUDP);
		m_socketUDP = 0;
	}

	for (int i = 0; i < m_TCPListeners; i++)
	{
		if (m_socketTCP[i])
		{
			closesocket(m_socketTCP[i]);
			m_socketTCP[i] = 0;
		}
	}
}

int NetBase::BindSocketUDP(SOCKET &_socket, sockaddr_in &_address, unsigned short _port, const char* _ip)
{
	//If reconnect
	if (_socket)
	{
		closesocket(_socket);
		_socket = 0;
	}

	//UDP
	memset(&_address, 0, sizeof(_address));
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = htonl(INADDR_ANY);
	_address.sin_port = htons(ADDR_ANY);

	if ((_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		printf("Create socket UDP FAILED: %d\n", (int)_socket);
		return -1;
	}
	if (bind(_socket, (struct sockaddr *) &_address, sizeof(_address)) < 0)
	{
		printf("Bind socket UDP FAILED: %d\n", WSAGetLastError());
		return -2;
	}

	//int bufferSize = 0;
	//int pointerSize = 4;
	//int err = getsockopt( _socket, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, &pointerSize);

	//setsockopt( m_socketUDP, IPPROTO_UDP, UDP_NODELAY, (char*)&i, sizeof(i));

	int iTimeout = 500;
	if (setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&iTimeout, sizeof(iTimeout)))
	{
		printf("Set socket option FAILED: %d\n",WSAGetLastError());
		return -3;
	}

	int option = 1024 * 128;
	if (setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (const char *)&option, sizeof(option)))
	{
		printf("Set socket option FAILED: %d\n",WSAGetLastError());
		return -3;
	}

	if (setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (const char *)&option, sizeof(option)))
	{
		printf("Set socket option FAILED: %d\n", WSAGetLastError());
		return -3;
	}

	inet_pton(AF_INET, _ip, &_address.sin_addr.s_addr);
	_address.sin_port = htons(_port);

	printf("Sending to UDP port %d\n", _address.sin_port);

	sockaddr_in addr;
	int size = sizeof(sockaddr);
	getsockname(_socket, (struct sockaddr *) &addr, &size);

	printf("Receiving on UDP port %d %d\n", htons(addr.sin_port), addr.sin_port);

	return 0;
}

int NetBase::BindSocketTCP(SOCKET &_socket, sockaddr_in &_address, unsigned short _port, const char* _ip)
{
	//TCP
	memset(&_address, 0, sizeof(_address));
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = htonl(INADDR_ANY);
	_address.sin_port = htons(ADDR_ANY);
	if ((_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printf("Create socket TCP FAILED: %d\n", WSAGetLastError());
		return 0;
	}
	// 		int i = 1;
	// 		setsockopt(m_socketTCP, IPPROTO_TCP, TCP_NODELAY, (char*)&i, sizeof(i));

	int iTimeout = 10000;
	if (setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&iTimeout, sizeof(iTimeout)))
		return -1;

	if (setsockopt(_socket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&iTimeout, sizeof(iTimeout)))
		return -2;

	int optval = 1;
	setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof optval);

	inet_pton(AF_INET, IP_ADRESS.c_str(), &_address.sin_addr.s_addr);
	_address.sin_port = htons(TCP_PORT);
	return 0;

}

void NetBase::SetSimulationLoss(int _val)
{
	if (m_lossChance == _val)
		return;

	printf("Loss is now %d\n", _val);
	m_lossChance = _val;
}

void NetBase::AddSystemMessage(const char* _str, int type)
{
	printf("%s", _str);
}

int NetBase::Connect()
{
	if (m_threadList.size() > 0)
		return -7;

	m_isConnecting = 1;
	m_connectionProblem = 0;

	printf("Connecting to %s\n", IP_ADRESS.c_str());

	if (BindSockets() < 0)
	{
		AddSystemMessage("Connection failed!", 12);
		m_isConnecting = 0;
		return -2;
	}

	for (int i = 0; i < m_TCPListeners; i++)
	{
		// Connect to server.
		int err = connect(m_socketTCP[i], (struct sockaddr *)&m_addressTCP, sizeof(m_addressTCP));

		if (err == SOCKET_ERROR)
		{
			int wsaErr = WSAGetLastError();
			printf("connect FAILED: %d\n", wsaErr);
			std::string errstr = "Connection blocked or server not running. Server router might not be port forwarded.";
			AddSystemMessage(errstr.c_str(), 12);
			m_isConnecting = 0;
			return wsaErr;
		}

		int iTimeout = 20000;
		if (setsockopt(m_socketTCP[i], SOL_SOCKET, SO_RCVTIMEO, (const char *)&iTimeout, sizeof(iTimeout)))
		{
			AddSystemMessage("Connection failed!", 10);
			m_isConnecting = 0;
			return -2;
		}

		printf("Connection established! %d\n",i);
	}

	StartThreads();

	m_isConnecting = 0;

	return 0;
}

void NetBase::Disconnect()
{
	printf("Sending disconnect message\n");

	if (IsConnected())
	{
		Pkt::X_Disconnect pkt;

		for (int i = 0; i < m_TCPListeners; i++)
		{
			MessageTCP(sizeof(Pkt::X_Disconnect), (char*)&pkt, i);
		}
		Sleep(50); //give time to send disconnect msg
	}

	if (m_socketUDP)
		closesocket(m_socketUDP);

	for (int i = 0; i < m_TCPListeners; i++)
	{
		if (m_socketTCP[0])
			closesocket(m_socketTCP[0]);
	}

	StopThreads();

	m_connectionProblem = 0;
}

void NetBase::StopThreads()
{
	if (m_threadList.size() > 0)
	{
		printf("Stopping threads wait...\n");
		m_quit = true;
		for (int i = 0; i < m_threadList.size(); i++)
		{
			m_threadList[i]->join();
		}
		m_threadList.clear();
		printf("Stopping threads ok\n");
		
	}
}

void NetBase::HandleRetransmissionTimer(bool _isRedundant, long long _originalTime)
{
	//long long RTT = Timers::Get().Time() - _originalTime;
	//static const int MAX_WAIT = 1000000;

	//if (_isRedundant && m_retransmissionSleepTime < MAX_WAIT)
	//{
	//	m_retransmissionSleepTime += long long(25 * m_retransmissionTimeAccelerator);
	//	//printf("Retransmission timeout is now %d\n", m_retransmissionSleepTime);
	//}
	//else if (m_retransmissionSleepTime > RTT*4)
	//{
	//	m_retransmissionSleepTime -= long long(50 * m_retransmissionTimeAccelerator);
	//	//printf("Retransmission timeout is now %d\n", m_retransmissionSleepTime);
	//}

	//if (_isRedundant == m_retransmissionLastDecision)
	//{
	//	m_retransmissionTimeAccelerator += 0.5f;
	//}
	//else
	//	m_retransmissionTimeAccelerator = 1.0f;

	//if (m_retransmissionSleepTime < RTT * 4)
	//	m_retransmissionSleepTime = RTT * 4;
	//if (m_retransmissionSleepTime > MAX_WAIT)
	//	m_retransmissionSleepTime = MAX_WAIT;
	//printf("RTT %lld - Sleep: %d\n", RTT, m_retransmissionSleepTime);
}

void NetBase::Destroy()
{
	Disconnect();
	WSACleanup();
	//m_netLogger.Destroy();
}

long long NetBase::GetPing()
{
	return m_ping;
}

int NetBase::InitNetwork()
{
	return InitWSA();
}
