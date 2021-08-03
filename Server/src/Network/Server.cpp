#include "Server.h"
#include "../Utility/Timers.h"
#include <ResolutionSettings.h>
#include <omp.h>

bool Server::m_newFrameReady[2] = { false };

Server::Server(VRInputs* _vrinput)
{
	m_VRInput = _vrinput;
	m_TCPListeners = 1;
	m_inputGraph.open("../content/output/inputFlow.txt");
}

Server::~Server()
{

	m_clientConnected = false;
	m_clientUDPAddr.sin_family = 0;

	for (int i = 0; i < 2; i++)
	{
		closesocket(m_clientSocket[i]);
	}

	Destroy();
	m_inputGraph.close();
}

int Server::ListenTCP_Join_ThreadLoop(void* ptr)
{
	Server* self = (Server*)ptr;

	while (!self->m_quit)
	{
		self->ListenForJoinTCP();
	}

	return 0;
}

int Server::InitNetwork()
{
	m_clientUDPAddr.sin_family = 0;

	int err = InitWSA();
	if(err >= 0)
		err = BindSockets();
	if (err >= 0)
		StartThreads();

	return err;
}

bool Server::ClientIsConnected()
{
	return m_clientConnected && m_clientUDPAddr.sin_family != 0;
}

bool Server::ForceNewRender()
{
	if (m_clientIsStuck[0] && m_clientIsStuck[1])
	{
		printf("Client is stuck\n");
		m_clientIsStuck[0] = false;
		m_clientIsStuck[1] = false;
		return true;
	}
	return false;
}

int Server::BindSocketUDP(SOCKET& _socket, sockaddr_in& _address, unsigned short _port, const char * _ip)
{
	m_quit = 0;

	//Init UDP
	memset(&_address, 0, sizeof(_address));
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = htonl(INADDR_ANY);
	_address.sin_port = htons(_port);

	int optval = 1;
	setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof optval);

	if ((_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		printf("Create socket FAILED: %d\n", (int)_socket);
		return -1;
	}
	if (bind(_socket, (struct sockaddr *) &_address, sizeof(_address)) < 0)
	{
		printf("Bind UDP socket FAILED: %d\n", WSAGetLastError());
		return -2;
	}
	int iTimeout = 10000;
	if (setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&iTimeout, sizeof(iTimeout)))
	{
		printf("Set socket option FAILED: %d\n", WSAGetLastError());
		return -3;
	}

	return 0;
}

int Server::BindSocketTCP(SOCKET& _socket, sockaddr_in& _address, unsigned short _port, const char * _ip)
{
	memset(&_address, 0, sizeof(_address));
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = htonl(INADDR_ANY);
	_address.sin_port = htons(_port);

	if ((_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printf("Create socket FAILED: %d\n", (int)_socket);
		return -1;
	}

	int optval = 1;
	setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof optval);
	setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof optval);

	if (bind(_socket, (struct sockaddr *) &_address, sizeof(_address)) < 0)
	{
		printf("Bind TCP socket FAILED: %d\n", WSAGetLastError());
		return -2;
	}

	return 0;
}

int Server::ListenForJoinTCP()
{
	sockaddr_in		incoming;
	bool connection = 1;

	while (!m_quit)
	{
		connection = !connection; //Toggle channel 0 and 1

		int received = listen(m_socketTCP[0], SOMAXCONN);

		if (received == SOCKET_ERROR)
			return -1;

		int addrLength = sizeof(incoming);
		m_clientSocket[connection] = accept(m_socketTCP[0], (sockaddr *)&incoming, (socklen_t*)&addrLength);

		if (m_clientSocket[connection] == INVALID_SOCKET)
		{
			continue;
		}

		//Give limited time to respond
		int iTimeout = 10000;
		if (setsockopt(m_clientSocket[connection], SOL_SOCKET, SO_RCVTIMEO, (const char *)&iTimeout, sizeof(iTimeout)))
		{
			printf("Set socket timeout FAILED: %d\n", WSAGetLastError());
			closesocket(m_clientSocket[connection]);
			continue;
		}

		char ip[33];
		memset(ip, 0, 33);
		inet_ntop(AF_INET, (void*)&incoming.sin_addr, ip, 33);
		printf("Client %d connected %s\n", connection, ip);
		m_clientConnected = true;

		if (connection == 0)
		{
			m_ThreadListenUDP = std::thread(ListenUDP_ThreadLoop, this);
			m_ThreadListenTCP = std::thread(ListenTCP_ThreadLoop, this);
			m_threadList.push_back(&m_ThreadListenUDP);
			m_threadList.push_back(&m_ThreadListenTCP);
		}
	}

	return 0;
}

int Server::MessageUDP(int _size, void* _data)
{
	//static int sentPackets = 0;

	if (m_clientUDPAddr.sin_family == 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		printf("Waiting for client to tell UDP adress...\n");
		return 0;
	}

	if (m_lossChance > 0)
	{
		if (rand() % m_lossChance == 0)
		{
			//printf("Simulated loss\n");
			return 0;
		}
	}

	int sent = sendto(m_socketUDP, (char*)_data, _size, 0, (struct sockaddr *)&m_clientUDPAddr, sizeof(m_clientUDPAddr));

	if (sent != _size)
	{
		int wsaerr = WSAGetLastError();
		printf("Failed to send UDP: %d\n", wsaerr);
		m_connectionProblem++;
		return wsaerr;
	}

	//sentPackets++;

	//printf("sent by server %d			\r", sentPackets);

	//m_netShaper.SendData(sent);

	//double t1 = omp_get_wtime();

	return 0;
}

void Server::PushSendAgainBuffer(int _eye, int _frame, int _packet, Pkt::Sv_FramePart* _data)
{
	m_sendAgainMaxIndex[_eye] = _frame;
	memcpy((void*)&m_sendAgainBuffer[_eye][_frame % FRAME_BUFFER_SIZE][_packet % PACKET_BUFFER_SIZE], _data, sizeof(Pkt::Sv_FramePart));
}

void Server::AckSendAgainBuffer(int _eye, int _frame, int _packet)
{
	m_sendAgainBuffer[_eye][_frame % FRAME_BUFFER_SIZE][_packet % PACKET_BUFFER_SIZE].IsUDP_IsAcked_IsRedundant = 1;
}

Pkt::Sv_FramePart* Server::GetSendAgainBufferData(int _eye, int _frame, int _packet)
{
	int slot = _frame % FRAME_BUFFER_SIZE;
	return (Pkt::Sv_FramePart*)&m_sendAgainBuffer[_eye][slot][_packet % PACKET_BUFFER_SIZE];
}

void Server::ListenUDP()
{
	sockaddr_in incoming;
	int addrLength = sizeof(incoming);

	char buffer[2048];
	//memset(buffer, -1, 2048);
	int received = recvfrom(m_socketUDP, buffer, 2048, 0, (sockaddr *)&incoming, (socklen_t*)&addrLength);

	Pkt::RVR_Header header = *(Pkt::RVR_Header*)&buffer[0];

	if (received != SOCKET_ERROR)
	{
		if (m_clientUDPAddr.sin_family == 0)
		{
			m_clientUDPAddr = incoming;
			m_clientUDPAddr.sin_family = AF_INET;
			printf("Client has port %d\n", htons(m_clientUDPAddr.sin_port));
		}

		if (header.PayloadType == Pkt::CL_INPUT)
		{
			const Pkt::Cl_Input& packet = *(Pkt::Cl_Input*)buffer;

			bool wasRedundant = false;

			if (Pkt::IsMatrix(packet.InputType))
			{
				//printf("Got input pkt %lld\n", packet.TimeStamp);
				if (m_VRInput->SetMatrix(packet) == false)
				{
					wasRedundant = true;
					//printf("Redundant input packet %u\n", packet.TimeStamp);
					//m_redundantInputPackets++;
					//m_inputPktsReceived[packet.PiggybackACK + 1] += 1;
					m_redundantInputPktsTotal++;
				}
				else
				{
					m_clientKeyPress = header.Misc;
					m_extrahackKeyPress = (int)header.DebugTimers[4];

					m_inputLossThisFrame = packet.SequenceNr;
					
					//if (m_redundantInputPktsTotal > 0)
					//{
					//	AckInput(packet.TimeStamp, packet.PiggybackACK + 1); //piggyback is previous frame id
					//}

					//m_redundantInputPktsTotal = 0;
				}

				if (packet.InputType == Pkt::InputTypes::MATRIX_HMD_POSE)
				{
					if(!wasRedundant)
					{
						//printf("%d, (%d %d)\n", packet.PiggybackACK, m_ACKedFrame[0], m_ACKedFrame[1]);

						if (packet.PiggybackACK > m_ACKedFrame[0])
						{
							m_ACKedFrame[0] = packet.PiggybackACK;
							OnFrameACK(0);
						}

						if (packet.PiggybackACK > m_ACKedFrame[1])
						{
							m_ACKedFrame[1] = packet.PiggybackACK;
							OnFrameACK(1);
						}
					}

					AckInput(packet.TimeStamp, packet.PiggybackACK + 1); //piggyback is previous frame id
				}

				static long long theFirstPkt = Timers::Get().Time();
				if (packet.SequenceNr == 0)
					m_inputGraph << (Timers::Get().Time() - theFirstPkt) << "	" << packet.TimeStamp << "	" << " " << "	";
				else
					m_inputGraph << (Timers::Get().Time() - theFirstPkt) << "	" << " " << "	" << packet.TimeStamp << "	";

				m_inputGraph << packet.SequenceNr << "\n";

				/*if (packet.ForceIDR)
				{
					m_lastFrameLost[0] = true;
					m_lastFrameLost[1] = true;
					printf("Loss %u\n", header.TimeStamp);
				}*/
			}
			//else if (Pkt::IsVector(packet.InputType)) //NO vectors with UDP
			//{
			//	glm::ivec2& resolution = *(glm::ivec2*)packet.Data;

			//	printf("VECTOR_RESOLUTION_%d %dxx%d\n", packet.InputType - Pkt::InputTypes::VECTOR_RESOLUTION_0, resolution.x, resolution.y);

			//	ResolutionSettings::Get().AddResolution(packet.InputType - Pkt::InputTypes::VECTOR_RESOLUTION_0, resolution);
			//}

			//Ack input packet
			/*Pkt::X_Ack ack;
			ack.TimeStamp = packet.TimeStamp;
			ack.IsUDP_IsAcked_IsRedundant = wasRedundant;
			ack.OriginalTime = packet.ClockTime;
			MessageUDP(sizeof(Pkt::X_Ack), &ack);*/



			//printf("Acking input %d\n", ack.TimeStamp);

			//m_netLogger.AddToGraph(received, "Input");
		}
		else if (header.PayloadType == Pkt::X_NACK_VECTOR)
		{
			const Pkt::X_NackVector& packet = *(Pkt::X_NackVector*)buffer;

			if (packet.TimeStamp > m_sendAgainMaxIndex[packet.Eye])
			{
				printf("Re-send doesnt exist yet! %d %u (Max %u)\n", packet.Eye, packet.TimeStamp, m_sendAgainMaxIndex[packet.Eye]);
				m_clientIsStuck[packet.Eye] = 1;
				Pkt::X_Performance perf;
				MessageUDP(sizeof(Pkt::X_Performance), &perf);
			}
			else
			{
				for (int i = 0; i < PACKET_BUFFER_SIZE; i++)
				{
					Pkt::Sv_FramePart& resend = *(Pkt::Sv_FramePart*)GetSendAgainBufferData(packet.Eye, packet.TimeStamp, i);

					if (packet.AckVector[i] == 0)
					{
						if (resend.FHead.DataSize == 0 || resend.TimeStamp != packet.TimeStamp)
						{
							printf("Invalid re-send request %d %d %d\n", resend.FHead.DataSize, resend.TimeStamp, packet.TimeStamp);
							break;
						}

						if(resend.FHead.Resends < 14)
							resend.FHead.Resends++;

						int size = sizeof(Pkt::RVR_Header) + sizeof(Pkt::Sv_FramePartHeader) + resend.FHead.DataSize;
						MessageUDP(size, &resend);

						//printf("Sending %d %d %d (%d) again. Size: %d\n", resend.FHead.Eye, resend.TimeStamp, resend.SequenceNr, resend.IsLastPkt, size);
					}

					if (resend.IsLastPkt)
						break;

					//else
					//{
					//	AckSendAgainBuffer(packet.Eye, packet.TimeStamp, packet.SequenceNr);
					//}
				}
			}

			//m_netLogger.AddToGraph(received, "Re-send");
		}
		else if (header.PayloadType == Pkt::X_NACK)
		{
			const Pkt::X_Nack& packet = *(Pkt::X_Nack*)buffer;

			if (packet.TimeStamp > m_sendAgainMaxIndex[packet.Misc])
			{
				printf("Re-send doesnt exist yet! %d %u (Max %u)\n", packet.Misc, packet.TimeStamp, m_sendAgainMaxIndex[packet.Misc]);
				m_clientIsStuck[packet.Misc] = 1;
				Pkt::X_Performance perf;
				MessageUDP(sizeof(Pkt::X_Performance), &perf);
			}
			else
			{
					Pkt::Sv_FramePart& resend = *(Pkt::Sv_FramePart*)GetSendAgainBufferData(packet.Misc, packet.TimeStamp, packet.SequenceNr);

					if (resend.FHead.DataSize == 0 || resend.TimeStamp != packet.TimeStamp)
					{
						printf("Invalid re-send request %d %d %d\n", resend.FHead.DataSize, resend.TimeStamp, packet.TimeStamp);
					}
					else
					{
						if (resend.FHead.Resends < 14)
							resend.FHead.Resends++;

						int size = sizeof(Pkt::RVR_Header) + sizeof(Pkt::Sv_FramePartHeader) + resend.FHead.DataSize;
						MessageUDP(size, &resend);

						//printf("Individual NACK %d %d %d (%d). Size: %d\n", resend.FHead.Eye, resend.TimeStamp, resend.SequenceNr, resend.IsLastPkt, size);
					}
			}

			//m_netLogger.AddToGraph(received, "Re-send");
		}
		else if (header.PayloadType == Pkt::X_CLOCK_SYNC)
		{
			Pkt::X_ClockSync& packet = *(Pkt::X_ClockSync*)buffer;

			packet.ServerTime = Timers::Get().Time();

			Timers::Get().SetPing(packet.LastPing);
			Timers::Get().SetServerTimeDiff(packet.LastServerDiff);

			MessageUDP(sizeof(Pkt::X_ClockSync), &packet);
		}
		else if (header.PayloadType == Pkt::X_ACK)
		{
			Pkt::X_Ack& packet = *(Pkt::X_Ack*)buffer;

			//printf("Client acked packet zero of frame %d, I have input %d\n", packet.TimeStamp, GetLastAckedInput());

			//Misc is eye

			if (packet.Misc > 1)
			{
				printf("Invalid ack %d %d %d\n", packet.Misc, packet.TimeStamp, packet.SequenceNr);
			}
			else
			{
				//printf("Client acked video: %d %d %d\n", packet.Misc, packet.TimeStamp, packet.SequenceNr);
				//AckSendAgainBuffer(packet.Misc, packet.TimeStamp, packet.SequenceNr);
				if (packet.TimeStamp > m_ACKedFrame[packet.Misc])
				{
					m_ACKedFrame[packet.Misc] = packet.TimeStamp;
					OnFrameACK(packet.Misc);
				}

			}

			//HandleRetransmissionTimer(packet.IsUDP_IsAcked_IsRedundant,packet.OriginalTime);
		}
		else if (header.PayloadType == Pkt::X_DUMMY)
		{
			
		}
	}
	else if (m_clientConnected)
	{
		int err = WSAGetLastError();
		if (err != WSAETIMEDOUT)
		{
			printf("UDP Connection problem %d %d\n", err, m_connectionProblem); //spam
			m_connectionProblem++;
		}
	}
}

unsigned int Server::GetLastAckedInput()
{
	return (unsigned int)m_VRInput->GetMatrixIndex();
}

//Stack of just one slot
int Server::GetClientKeyPress()
{
	int key = m_clientKeyPress;
	m_clientKeyPress = 0;
	return key;
}

int Server::GetExtraClientKeyPress()
{
	int key = m_extrahackKeyPress;
	m_extrahackKeyPress = 0;
	return key;
}

int Server::GetInputLossThisFrameReset()
{
	int was = m_inputLossThisFrame.exchange(0); //reset
	 
	return was;
}

//
//void Server::EnsureRetransmissions(int _eye, unsigned int _timeStamp)
//{
//	while (true)
//	{
//		int neededPackets = 0;
//		int ackedPackets = 0;
//
//		std::this_thread::sleep_for(std::chrono::microseconds(m_retransmissionSleepTime));
//
//		for (int i = 0; i < PACKET_BUFFER_SIZE; i++)
//		{
//			Pkt::Sv_FramePart* pkt = GetSendAgainBufferData(_eye, _timeStamp, i);
//
//			//Is old packet (shouldnt happen)
//			if (pkt->TimeStamp != _timeStamp)
//			{
//				printf("Invalid timestamp in sendagain buffer %d %d %d %d\n", _eye, pkt->TimeStamp, _timeStamp, i);
//				continue;
//			}
//
//			neededPackets++;
//
//			//Is already acked
//			if (pkt->IsUDP_IsAcked_IsRedundant)
//			{
//				ackedPackets++;
//
//				if (pkt->IsLastPkt)
//				{
//					break;
//				}
//
//				continue;
//			}
//
//			int size = sizeof(Pkt::RVR_Header) + sizeof(Pkt::Sv_FramePartHeader) + pkt->FHead.DataSize;
//			MessageUDP(size, pkt);
//
//			printf("Retransmitting %d %d %d %d (Wait: %d)\n", _eye, pkt->TimeStamp, pkt->SequenceNr, pkt->IsLastPkt, m_retransmissionSleepTime);
//
//			if (pkt->IsLastPkt)
//			{
//				break;
//			}
//		}
//
//		if (neededPackets == ackedPackets)
//		{
//			if (neededPackets == 0)
//				printf("huh %d %d\n", neededPackets, ackedPackets);
//			break;
//		}
//	}
//}

void Server::OnFrameACK(int _eye)
{
	//Notify render thread that a new frame is ready
	std::lock_guard<std::mutex> guard(m_runMutex[_eye]);
	m_newFrameReady[_eye] = true;
	m_newFrameCond[_eye].notify_one();
}

bool Server::Predicate0()
{
	return m_newFrameReady[0];
}

bool Server::Predicate1()
{
	return m_newFrameReady[1];
}

void Server::EnsureRetransmissions(int _eye, unsigned int _timeStamp, long long _timerStart)
{
	//m_frameToBeACKed[_eye] = _timeStamp;

	//Pkt::Sv_FramePart* pkt = GetSendAgainBufferData(_eye, _timeStamp, 0);

	//long long t0 = _timerStart;
	long long lastRequest = _timerStart;
	//int seqNr = 0;
	//long long innerTime = Timers::Get().Time();
	int resends = 0;

	bool (*function)();

	if (_eye == 0)
		function = Predicate0;
	else
		function = Predicate1;

	while (m_ACKedFrame[_eye] < _timeStamp && !m_quit)
	{
		//Timers::Get().DeadlockCheck(_timerStart, "Server ensure retransmission loop");
		//Is old packet (shouldnt happen)
		//if (pkt->TimeStamp != _timeStamp)
		//{
		//	printf("Invalid timestamp in sendagain buffer %d %d %d %d\n", _eye, pkt->TimeStamp, _timeStamp, 0);
		//	continue;
		//}

		{
			std::unique_lock<std::mutex> waitLock(m_runMutex[_eye]);
			//if (m_newFrameCond[_eye].wait_for(waitLock, std::chrono::microseconds(100), [this](int _eye) { return m_newFrameReady[_eye]; }))
			if (m_newFrameCond[_eye].wait_for(waitLock, std::chrono::microseconds(100), function))
			{
				m_newFrameReady[_eye] = false;
			}
		}

		//Is already acked
		if (m_ACKedFrame[_eye] >= _timeStamp)
		{
			break;
		}

		//Is already acked
		//if (pkt->IsUDP_IsAcked_IsRedundant)
		//{
		//	break;
		//}

		long long timeConsumed = Timers::Get().Time() - lastRequest;


		if (m_statsACK.IsSignificantWaitGamma(timeConsumed))
		{
			Pkt::Sv_FramePart* pkt = GetSendAgainBufferData(_eye, _timeStamp, 0);

			if (pkt->FHead.Resends >= 15)
				pkt->FHead.Resends = 15;

			pkt->FHead.Resends++;
			int size = sizeof(Pkt::RVR_Header) + sizeof(Pkt::Sv_FramePartHeader) + pkt->FHead.DataSize;
			MessageUDP(size, pkt);
			//printf("Retransmitting ZERO %d %d %d %d %lld %d\n", _eye, pkt->TimeStamp, pkt->SequenceNr, pkt->IsLastPkt, timeConsumed, pkt->FHead.Resends);
			lastRequest = Timers::Get().Time();
			resends++;

			//printf("Resending packet zero! %u\n", pkt->TimeStamp);
		}
	}


	//Client has acked the first packet of the new frame, this means that it has received ack for input, so we can reset here
	//m_inputPktsReceivedSinceLast = 0;

	long long ackTime = Timers::Get().Time() - _timerStart;

	//printf("Waited %lld\n", ackTime);

	//printf("Time for ACK %lld\n", ackTime);
	if(resends == 0)
		m_statsACK.AddSample(ackTime, m_ping);
}

void Server::NackInput(unsigned int _timeStamp)
{
	Pkt::X_Nack pkt;
	pkt.TimeStamp = _timeStamp;
	MessageUDP(sizeof(Pkt::X_Nack), &pkt);
}


void Server::AckInput(unsigned int _inputTimeStamp, unsigned int _frameTimestamp)
{
	Pkt::Sv_Ack pkt;
	pkt.TimeStamp = _frameTimestamp;
	pkt.InputPktTimestamp = _inputTimeStamp;

	/*if (m_inputPktsReceived.size() > 150)
	{
		unsigned int index = _frameTimestamp - 10;

		pkt.InputPktTimestamp = index;
		pkt.InputPktsRedundant = m_inputPktsReceived[index];
	}

	if (m_inputPktsReceived.size() > 400)
	{
		m_inputPktsReceived.erase(m_inputPktsReceived.begin());
	}*/
	pkt.InputPktsRedundant = m_redundantInputPktsTotal;

	MessageUDP(sizeof(Pkt::Sv_Ack), &pkt);
}

//int Server::GetRedundantInputPkts()
//{
//	int pkts = m_redundantInputPackets;
//	m_redundantInputPackets = 0;
//	return pkts;
//}

//void Server::EnsureRetransmissions(int _eye, unsigned int _timeStamp)
//{
//	long long t0 = Timers::Get().Time();
//
//	while (true)
//	{
//		int neededPackets = 0;
//		int ackedPackets = 0;
//
//		std::this_thread::sleep_for(std::chrono::microseconds(500));
//
//		long long timeConsumed = Timers::Get().Time() - t0;
//
//		for (int i = 0; i < PACKET_BUFFER_SIZE; i++)
//		{
//			Pkt::Sv_FramePart* pkt = GetSendAgainBufferData(_eye, _timeStamp, i);
//
//			//Is old packet (shouldnt happen)
//			if (pkt->TimeStamp != _timeStamp) 
//			{
//				printf("Invalid timestamp in sendagain buffer %d %d %d %d\n",_eye, pkt->TimeStamp, _timeStamp,i);
//				continue;
//			}
//
//			neededPackets++;
//
//			//Is already acked
//			if (pkt->IsUDP_IsAcked_IsRedundant)
//			{
//				ackedPackets++;
//
//				if (pkt->IsLastPkt)
//				{
//					break;
//				}
//
//				continue;
//			}
//
//			if (timeConsumed > m_retransmissionSleepTime)
//			{
//				int size = sizeof(Pkt::RVR_Header) + sizeof(Pkt::Sv_FramePartHeader) + pkt->FHead.DataSize;
//				MessageUDP(size, pkt);
//				printf("Retransmitting %d %d %d %d (Wait: %d)\n", _eye, pkt->TimeStamp, pkt->SequenceNr, pkt->IsLastPkt, m_retransmissionSleepTime);
//				t0 = Timers::Get().Time();
//			}
//
//			if (pkt->IsLastPkt)
//			{
//				break;
//			}
//		}
//
//		if (neededPackets == ackedPackets)
//		{
//			if (neededPackets == 0)
//				printf("huh %d %d\n", neededPackets, ackedPackets);
//			break;
//		}
//	}
//}


void Server::ListenTCP(int _channel)
{
	Pkt::RVR_Header header;
	int received = Recv(m_clientSocket[_channel], (char*)&header, sizeof(Pkt::RVR_Header)); //Read header type of 1bytes

	if (received > 0)
	{
		m_connectionProblem = 0;
		//printf("pkt\n");
		if (header.PayloadType == Pkt::CL_INPUT)//Read packet with 4 byte incremented
		{
			Pkt::Cl_Input packet;
			char* ptr = (char*)&packet;
			memcpy(ptr, &header, sizeof(Pkt::RVR_Header));
			ptr += sizeof(Pkt::RVR_Header);

			received += Recv(m_clientSocket[_channel], ptr, sizeof(Pkt::Cl_Input) - sizeof(Pkt::RVR_Header));

			if (received == sizeof(Pkt::Cl_Input))
			{
				m_clientKeyPress = header.Misc;

				if (Pkt::IsMatrix(packet.InputType))
				{
					//printf("Got input pkt %lld\n", packet.TimeStamp);
					m_VRInput->SetMatrix(packet);
					/*if (packet.ForceIDR)
					{
						m_lastFrameLost[0] = true;
						m_lastFrameLost[1] = true;
						printf("Loss %u\n", header.TimeStamp);
					}*/
				}
				else if (Pkt::IsVector(packet.InputType))
				{
					glm::ivec2& resolution = *(glm::ivec2*)packet.Data;

					printf("VECTOR_RESOLUTION_%d %dxx%d\n", packet.InputType - Pkt::InputTypes::VECTOR_RESOLUTION_0, resolution.x, resolution.y);
					//ResolutionSettings::Get().AddResolution(packet.InputType - Pkt::InputTypes::VECTOR_RESOLUTION_0, resolution); //TCP ensures the correct order anyway
					ResolutionSettings::Get().AddResolution(resolution);

					printf("FRAME_RATE %d\n", packet.Misc);
					StatCalc::SetParameter("FRAME_RATE", (double)packet.Misc);
				}
			}
		}
		else if (header.PayloadType == Pkt::CL_LOSS)//Read packet with 4 byte incremented
		{
			int index = header.TimeStamp % FRAME_BUFFER_SIZE;
			m_lastFrameLost[header.IsLastPkt] = true; //Marker is now eye
			printf("CL_LOSS %d %u\n", header.IsLastPkt,header.TimeStamp);
		}
		else if (header.PayloadType == Pkt::CL_LENS_DATA)//Read packet with 4 byte incremented
		{
			Pkt::Cl_LensData packet;
			char* ptr = (char*)&packet;
			memcpy(ptr, &header, sizeof(Pkt::RVR_Header));
			ptr += sizeof(Pkt::RVR_Header);

			//Timestamp is size or number of lensdata vertices
			int result = Recv(m_clientSocket[_channel], ptr, sizeof(VertexDataLens) * packet.TimeStamp);

			if (result != -1)
			{
				for (unsigned int i = 0; i < packet.TimeStamp; i++)
				{
					m_VRInput->AddLensVertex(packet.Data[i]);
				}

				if (packet.IsLastPkt)
				{
					m_VRInput->SetLensDataOK();
				}
			}
			else
			{
				m_connectionProblem++;
				m_quit = true;
			}
		}
		else if (header.PayloadType == Pkt::CL_HIDDEN_AREA_MESH)//Read packet with 4 byte incremented
		{
			Pkt::Cl_HiddenAreaMesh packet;
			char* ptr = (char*)&packet;
			memcpy(ptr, &header, sizeof(Pkt::RVR_Header));
			ptr += sizeof(Pkt::RVR_Header);

			//Timestamp is triangle count, IslastPkt is eye ID
			HiddenAreaMesh& mesh = m_VRInput->AllocateHiddenAreaMesh(packet.IsLastPkt, packet.TimeStamp);

			//Timestamp is triangle count
			int result = Recv(m_clientSocket[_channel], mesh.VertexData, sizeof(vr::HmdVector2_t) * packet.TimeStamp * 3);

			if (result / sizeof(glm::vec2) / 3 == packet.TimeStamp)
			{
				printf("Hidden area mesh %d is OK, %d triangles\n", packet.IsLastPkt, packet.TimeStamp);
			}
			else
			{
				printf("ERROR!\nReceived hidden area mesh for eye %d with triangle count %d\n", packet.IsLastPkt, packet.TimeStamp);
				printf("Got %d Bytes of data aka %zd triangles\n", result, result / sizeof(glm::vec2) / 3);
			}
		}
		else if (header.PayloadType == Pkt::CL_KEY_PRESS)//Read packet with 4 byte incremented
		{

			m_clientKeyPress = header.TimeStamp;
		}
	}
	else if (m_clientConnected)
	{
		int err = WSAGetLastError();
		if (err != WSAETIMEDOUT)
		{
			printf("Connection problem %d %d\n", err, m_connectionProblem); //spam
			m_connectionProblem++;
			m_quit = true;
		}
	}
}

bool Server::IsLastFrameLost(int _eye)
{
	bool acked = m_lastFrameLost[_eye];

	if(acked)
		m_lastFrameLost[_eye] = false;

	return acked;
}

void Server::StartThreads()
{
	if (m_threadList.size() == 0)
	{
		m_ThreadListenTCP_join = std::thread(ListenTCP_Join_ThreadLoop, this);

		m_threadList.push_back(&m_ThreadListenTCP_join);
	}
}

void Server::Update()
{
}

bool Server::CheckForDisconnect()
{
	if (m_connectionProblem > 0)
	{
		printf("Disconnecting client...\n");
		DisconnectClient();
		m_connectionProblem = 0;
		printf("Disconnecting client complete\n");
		return true;
	}
	return false;
}

void Server::DisconnectClient()
{
	Pkt::X_Disconnect pkt;

	for (int i = 0; i < m_TCPListeners; i++)
	{
		MessageTCP(sizeof(Pkt::X_Disconnect), (char*)&pkt, i);
	}

	Sleep(2000); //give time to send disconnect msg

	m_clientConnected = false;
	m_clientUDPAddr.sin_family = 0;

	for (int i = 0; i < 2; i++)
	{
		closesocket(m_clientSocket[i]);
	}

	CloseSockets();

	StopThreads();
}

void Server::RestartListen()
{
	BindSockets();

	StartThreads();
}

int Server::MessageTCP(int _size, void* _data, int _channel)
{
	int sent = 0;

	while (sent < _size)
	{
		int err = send(m_clientSocket[_channel], (const char*)_data, _size, 0);
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
