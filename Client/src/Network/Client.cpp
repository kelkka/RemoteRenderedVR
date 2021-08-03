/*
* 
*/

#include "Client.h"
#include "../Utility/Timers.h"
#include <EncodedFrame.h>

#include <omp.h>
#include <iomanip>

long long Client::m_beginTime=0;

//#define PRINT_FRAME_PARTS

Client::Client()
{
	m_TCPListeners = 2;

	printf("Maximum UDP packet size is %zd\n", sizeof(Pkt::Sv_FramePart));
	printf("Maximum X_NackVector packet size is %zd\n", sizeof(Pkt::X_NackVector));

	//Init buffers
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < FRAME_BUFFER_SIZE; j++)
		{
			for (int k = 0; k < PACKET_BUFFER_SIZE; k++)
			{
				m_eye[i].Queue[j][k] = nullptr;
			}
		}

		for (int j = 0; j < EyeData::EYE_DATA_MEMORY_SLOTS; j++)
		{
			m_eye[i].ParallelData[j].FirstPktRecvTime = Timers::Get().Time();
		}
	}

	m_statsResendPktDelay.Initialize(1000);
#ifdef PRINT_FRAME_PARTS
	m_framePartsGraph.open("../content/frameParts.txt");
#endif
	m_beginTime = Timers::Get().Time();
}

Client::~Client()
{
#ifdef PRINT_FRAME_PARTS
	m_framePartsGraph.close();
#endif
	Destroy();
}

int Client::MessageUDP(int _size, void* _data)
{
	if (m_lossChance > 0)
	{
		if (rand() % m_lossChance == 0)
		{
			//printf("Simulated loss\n");
			return 0;
		}
	}

	int err = sendto(m_socketUDP, (char*)_data, _size, 0, (struct sockaddr *)&m_addressUDP, sizeof(m_addressUDP));
	if (err != _size)
	{
		int wsaerr = WSAGetLastError();
		printf("Failed to send UDP: %d\n", wsaerr);
		m_connectionProblem++;
		return wsaerr;
	}

	return 0;
}

//This thread just polls data from winsock and puts packets in m_packetBufferUDP while notifying the unpacker.
//This is just to make sure the winsock buffer is cleared ASAP to avoid UDP packet loss on overflow.
//The buffer is huge now though, so this might be redundant.
void Client::ListenUDP()
{
	char* buffer = (char*)m_packetBufferUDP[m_packetBufferIndexUDP.load() % RAW_PACKET_BUFFER_SIZE_UDP];
	
	int received = recvfrom(m_socketUDP, buffer, LARGEST_PACKET_SIZE, 0, NULL, NULL);

	if (received != SOCKET_ERROR)
	{
		m_packetBufferIndexUDP++;
		std::lock_guard<std::mutex> guard(m_unpackMutex);
		m_packetIsFresh = true;
		m_unpackCondition.notify_one();

		//m_framePartGraphMutex.lock();
		//m_framePartsGraph << (Timers::Get().Time() - m_beginTime) << "						" << 3 << "\n";
		//m_framePartGraphMutex.unlock();
	}
	else
	{
		sockaddr_in addr;
		int size = sizeof(sockaddr_in);
		getsockname(m_socketUDP, (struct sockaddr *) &addr, &size);

		printf("Receiving on UDP port %d %d\n", htons(addr.sin_port), addr.sin_port);

		buffer[0] = -111;
		int err = WSAGetLastError();
		if (err != WSAETIMEDOUT && err != 0)
		{
			printf("Connection problem %d %d\n", err, m_connectionProblem); //spam
			m_connectionProblem++;
			m_quit = true;
		}
	}
}

//Unpacks a frame part from UDP packet
void Client::RecvSvFramePart(Pkt::Sv_FramePart & packet, int _dataSize)
{
	if (packet.FHead.Eye > 1 || packet.FHead.DataSize <= 0 || packet.FHead.DataSize > FRAME_PAYLOAD_SIZE)
	{
		printf("Bad Sv_FramePart packet, Eye %d, Size %d\n", packet.FHead.Eye, packet.FHead.DataSize);
		return;
	}

	//packet.DebugTimers[4] = Timers::Get().Time() - packet.DebugTimers[2];

	unsigned int queueIndex = packet.TimeStamp % FRAME_BUFFER_SIZE;
	unsigned int eyeDataIndex = packet.TimeStamp % EyeData::EYE_DATA_MEMORY_SLOTS;

	if (packet.SequenceNr >= PACKET_BUFFER_SIZE)
	{
		packet.SequenceNr = PACKET_BUFFER_SIZE - 1;
		printf("Packet index larger than buffer!\n");
		system("pause");
	}

	//while (m_eye[packet.FHead.Eye].RawPosR.load() + FRAME_BUFFER_SIZE <= packet.TimeStamp && !m_quit)
	//{
	//	printf("Reader is too slow! %d %u\n", m_eye[packet.FHead.Eye].RawPosR.load(), packet.TimeStamp);
	//	std::this_thread::sleep_for(std::chrono::microseconds(100000));
	//	system("pause");
	//}

	long long timeNow = Timers::Get().Time();

	bool isRedundant = false;

	m_eye[packet.FHead.Eye].QueueMutex.lock();

	if (m_eye[packet.FHead.Eye].Queue[queueIndex][packet.SequenceNr] != nullptr) 
	{
		isRedundant = (m_eye[packet.FHead.Eye].Queue[queueIndex][packet.SequenceNr]->TimeStamp == packet.TimeStamp); //Packet already is here
	}
	if (m_eye[packet.FHead.Eye].RawPosR > packet.TimeStamp)
	{
		isRedundant = true; //This has already been read
	}

	//Copy packet!
	if (isRedundant == false)
	{
		m_eye[packet.FHead.Eye].Queue[queueIndex][packet.SequenceNr] = &packet;	//queue slot now points to recvd packet

		if (m_eye[packet.FHead.Eye].ParallelData[eyeDataIndex].PacketsRecvdThisFrame == 0)
		{
			m_eye[packet.FHead.Eye].GoDecoder(); //Start decoder thread so it is ready to NACK
			m_eye[packet.FHead.Eye].ParallelData[eyeDataIndex].FirstPktRecvTime = timeNow;
		}

		m_eye[packet.FHead.Eye].ParallelData[eyeDataIndex].PacketsRecvdThisFrame++;
	}

	m_eye[packet.FHead.Eye].QueueMutex.unlock();

	//unsigned int currentWrite = m_eye[packet.FHead.Eye].RawPosW.load();

	//Packet has some other timestamp, adjust
	//if (packet.TimeStamp > currentWrite)
	//{
	//	printf("Missing final packet from previous? %u %u\n", currentWrite, packet.TimeStamp);
	//	//m_eye[packet.FHead.Eye].RawPosW = packet.TimeStamp;
	//	
	//}
	//else if(packet.IsLastPkt && currentWrite == packet.TimeStamp) //Next frame expected, all good
	//{
	//	//This is the final packet, so the whole frame should have arrived by now, 
	//	//Here we set stuff that is done on a frame-by-frame basis

	//	m_eye[packet.FHead.Eye].RawPosW.fetch_add(1);

	//	//Misc carries lost input packets for now...
	//	if (packet.Misc > 0)
	//	{
	//		m_lossInPrevInput = packet.Misc;
	//		//printf("Loss in input!\n");
	//	}

	//	//We need this many packets to construct the frame
	//	m_eye[packet.FHead.Eye].ParallelData[eyeDataIndex].PacketsNeededThisFrame = packet.SequenceNr + 1;
	//}

	if (isRedundant == false)
	{
		if (packet.IsLastPkt)
		{
			//Misc carries lost input packets for now...
			if (packet.Misc > 0)
			{
				m_lossInPrevInput = packet.Misc;
				//printf("Loss in input!\n");
			}

			//We need this many packets to construct the frame
			m_eye[packet.FHead.Eye].ParallelData[eyeDataIndex].PacketsNeededThisFrame = packet.SequenceNr + 1;
		}

		//Received all packets without loss? Then kick off decoder, because it will be sleeping to avoid busy wait
		if (m_eye[packet.FHead.Eye].IsAllPacketsReceivedThisFrame(eyeDataIndex))
		{
			m_eye[packet.FHead.Eye].GoDecoder();

			//Statistics on how long it takes to construct a frame, from part 0 to last.
			if (packet.FHead.Resends == 0)
				m_statsResendPktDelay.AddSample(timeNow - m_eye[packet.FHead.Eye].ParallelData[eyeDataIndex].FirstPktRecvTime, m_ping);

			//Timers::Get().PushLongLong(packet.FHead.Eye, T_NET_FIN, packet.TimeStamp, timeNow); //network finish
		}
	}

	//Is this part a re-transmission?
	if (packet.FHead.Resends > 0 || isRedundant) //Re-sent
	{
		//printf("Got resend %d %u %u (%d) (Redundant: %d)\n", packet.FHead.Eye, packet.TimeStamp, packet.SequenceNr, packet.IsLastPkt, isRedundant);
		if (isRedundant)
		{
			Timers::Get().AddLongLong(packet.FHead.Eye, T_VIDEO_REDUNDANT, packet.TimeStamp, 1);
		}
		else
		{
			Timers::Get().AddLongLong(packet.FHead.Eye, T_VIDEO_RE_SENDS, packet.TimeStamp, packet.FHead.Resends);
		}
	}

	//Is this the first packet?
	if (packet.SequenceNr == 0)
	{
		if (!isRedundant)
		{
			m_level = packet.FHead.Level;

			Timers::Get().PushLongLong(packet.FHead.Eye, T_INPUT_BEGIN, packet.TimeStamp, packet.DebugTimers[EncodedFrame::T_0_INPUT_SEND]); //input
			Timers::Get().PushLongLong(packet.FHead.Eye, T_RENDER_BEGIN, packet.TimeStamp, packet.DebugTimers[EncodedFrame::T_1_RENDER_START]); //begin
			Timers::Get().PushLongLong(packet.FHead.Eye, T_ENC_BEGIN, packet.TimeStamp, packet.DebugTimers[EncodedFrame::T_2_ENCODER_START]); //render
			Timers::Get().PushLongLong(packet.FHead.Eye, T_NET_BEGIN, packet.TimeStamp, packet.DebugTimers[EncodedFrame::T_3_NETWORK_START]); //network start
		}

		//ACK packet zero
		m_ACKpkt.TimeStamp = packet.TimeStamp;
		m_ACKpkt.SequenceNr = packet.SequenceNr;
		m_ACKpkt.Misc = packet.FHead.Eye;
		MessageUDP(sizeof(Pkt::X_Ack), &m_ACKpkt);
	}

	m_eye[0].BandwidthMutex.lock();
	Timers::Get().AddLongLong(0, T_BANDWIDTH, packet.TimeStamp, _dataSize); //bandwidth
	m_eye[0].BandwidthMutex.unlock();

}

//The decoder thread resides in here waiting for data from network.
int Client::CopyFBBuffer(unsigned char* _mem, unsigned char _eye, int _size)
{
	if (m_quit == true)
	{
		return -2;
	}

	unsigned int readPosRaw = m_eye[_eye].RawPosR;
	unsigned int read = readPosRaw % FRAME_BUFFER_SIZE;

	long long jitterWait = Timers::Get().Time();
	long long startTime = jitterWait;

	long long syncWait = 0;
	int reSendRequests = 0;

	int eyeDataIndex = readPosRaw % EyeData::EYE_DATA_MEMORY_SLOTS;

	//
	if (m_eye[_eye].SeqNr == 0)
	{
		//No new packets yet, read is same as write
		while (m_quit == false)
		{
			long long t0 = Timers::Get().Time();
			int receivedPkts = 0;
			bool isComplete = true;
			int holeAt = -1;
			bool holeTest = false;
			bool lastPacketHasArrived = false;

			Pkt::X_NackVector sendAgain;
			sendAgain.Eye = _eye;
			sendAgain.TimeStamp = readPosRaw;

			//Check for holes in received packets
			m_eye[_eye].QueueMutex.lock();
			for (int i = m_eye[_eye].SeqNr; i < PACKET_BUFFER_SIZE; i++)
			{
				Pkt::Sv_FramePart* packet = (Pkt::Sv_FramePart*)m_eye[_eye].Queue[read][i];

				bool isMissing = true;

				if (packet != nullptr)
				{
					if (packet->TimeStamp == readPosRaw)
					{
						sendAgain.AckVector[i] = 1; //Exists
						receivedPkts++;
						isMissing = false;

						//There is a hole before this one
						if (holeAt != -1)
						{
							holeTest = true;
						}

						if (packet->IsLastPkt)
						{
							lastPacketHasArrived = true;
							break;
						}
					}
					else
					{
						m_eye[_eye].Queue[read][i] = nullptr;
					}
				}

				if (isMissing)
				{
					sendAgain.AckVector[i] = 0; //Missing
					isComplete = false;

					//Dont steal another hole
					if (holeAt == -1)
					{
						holeAt = i;
					}
				}
			}
			m_eye[_eye].QueueMutex.unlock();

			//There was no missing packet, so decoder is now free to copy data
			if (isComplete)
			{
#ifdef PRINT_FRAME_PARTS
				m_framePartGraphMutex.lock();
				m_framePartsGraph << (Timers::Get().Time() - m_beginTime) << "				" << 0 << "\n";
				m_framePartGraphMutex.unlock();
#endif
				//Complete
				break;
			}

			//There is some missing packet, so we end up here and have to do re-transmissions

			long long timeSinceLastPkt = Timers::Get().Time() - m_eye[_eye].ParallelData[eyeDataIndex].FirstPktRecvTime;

			if (m_serverIsUsingUDP == true)
			{
				bool triggerNackNow = false;

				if (timeSinceLastPkt > 1000000)
				{
					printf("Extra timeout! waiting for %d\n", readPosRaw);
					m_eye[_eye].ParallelData[eyeDataIndex].FirstPktRecvTime = Timers::Get().Time();

					//Most likely no longer needed, but not sure
					if (readPosRaw == 1)
					{
						SendInputPacket();
					}
				}

				bool isSignificantWait = false;

				//If the missing packet is a hole, i.e. the final one has arrived, it is NACKed immediately one time
				if (holeTest && lastPacketHasArrived && reSendRequests == 0)
				{
					triggerNackNow = true; //Will trigger NACK
				}

				if (receivedPkts > 0)
				{
					if (reSendRequests == 0) 
					{
						//NACK not yet sent, use gamma distribution to find out when we should do NACK
						if (m_statsResendPktDelay.IsSignificantWaitGamma(timeSinceLastPkt))
						{
							isSignificantWait = true;
						}
					}
					else
					{
						//Already NACked once, if packets are still missing, the network situation is probably pretty bad and we just do exponential backoff.
						if (timeSinceLastPkt > Timers::Get().Ping() * 2.0f * reSendRequests)
						{
							isSignificantWait = true;
						}
					}
				}

				if (((
					receivedPkts > 0 &&												//Has gotten at least one packet
					isSignificantWait) ||											//Unusually high time spent to get all packets
					triggerNackNow) &&												//OR Extra timeout if hole or an extreme amount of time passed
					m_eye[!_eye].RawPosR >= readPosRaw)								//This read is not ahead of the other eye
				{
					//NACK is sent here
					MessageUDP(sizeof(Pkt::X_NackVector), &sendAgain);
					reSendRequests++;
					m_eye[_eye].ParallelData[eyeDataIndex].FirstPktRecvTime = Timers::Get().Time();
#ifdef PRINT_FRAME_PARTS
					m_framePartGraphMutex.lock();
					m_framePartsGraph << (Timers::Get().Time() - m_beginTime) << "				" << _eye + 1 << "\n";
					m_framePartGraphMutex.unlock();
#endif

				}
				else
				{
					//More waiting, no NACK trigger
				}
			}

			//This is some optimization that may or may not be needed still. 
			//If one packet has arrived, and there was no significant wait, the thread will stay alert and not go to sleep, anticipating the complete frame very soon
			bool stayAlert = false;
			if (m_eye[_eye].ParallelData[eyeDataIndex].PacketsRecvdThisFrame.load() > 0)
			{
				if (!m_statsResendPktDelay.IsSignificantWaitGamma(timeSinceLastPkt) && reSendRequests == 0)
				{
					stayAlert = true;
				}
			}

			//Avoids busy wait by conditional sleep, is woken up by unpacker if the final packet arrives
			if (stayAlert == false)
			{
				m_eye[_eye].CondWait(10000);
			}

			long long realWait = Timers::Get().Time() - t0;

			if (receivedPkts > 0)
				syncWait += realWait;

			if (Timers::Get().DeadlockCheck(startTime, "Client video packet loop"))
			{
				printf("Client is stuck! (Eye: %d Stamp: %d Wait: %lld Acked: %d Extra: %d Last: %lld, Needed: %d)\n", _eye, readPosRaw, syncWait, receivedPkts, 0, timeSinceLastPkt, m_eye[_eye].ParallelData[eyeDataIndex].PacketsNeededThisFrame);
				printf("Input pkt ID: %u Piggyback: %u InputpktACK %lld InputIndex %lld\n", m_inputPacket.TimeStamp, m_inputPacket.PiggybackACK, m_inputPacketAck, m_inputIndex);
				SendInputPacket();
				//system("pause");
				startTime = Timers::Get().Time();
			}
		}

		m_eye[_eye].Reset(eyeDataIndex);
	}
	else
	{
		//printf("saved seqnr %d\n", m_eye[_eye].SeqNr);
	}

	//Saves the first packet to text, maybe needed some time in the future

	//if (m_eye[_eye].FrameID == 0 && m_eye[_eye].ReadFromPkt == 0)
	//{
		//firstFrameHack = true;
		//_size = 36;
		//m_eye[_eye].ReadFromPkt = 36;
		//std::ofstream outHeader("../content/headerData.txt", std::ofstream::out);

		//while (seqNr < PACKET_BUFFER_SIZE && !m_quit)
		//{
		//	//Timers::Get().DeadlockCheck(startTime, "Client packet copy loop");

		//	m_eye[_eye].QueueMutex.lock();
		//	Pkt::Sv_FramePart* packetPtr = (Pkt::Sv_FramePart*)m_eye[_eye].Queue[read][seqNr];

		//	if (packetPtr == nullptr)
		//	{
		//		m_eye[_eye].QueueMutex.unlock();
		//		break;
		//	}

		//	Pkt::Sv_FramePart packet = *packetPtr;
		//	m_eye[_eye].QueueMutex.unlock();

		//	//Headers: Eth2 + IPv4 + UDP + FramePart + Data
		//	static const int HEADERS_SIZE = 14 + 20 + 8 + sizeof(Pkt::Sv_FramePartHeader);
		//	int dataToCopy = packet.FHead.DataSize;

		//	for (int i = 0; i < packet.FHead.DataSize - 3; i++)
		//	{
		//		char* temp = (char*)&packet.Data[i];

		//		if (temp[0] == 0 &&
		//			temp[1] == 0 &&
		//			temp[2] == 0 &&
		//			temp[3] == 1)
		//		{
		//			break;
		//		}
		//		_size++;
		//	}

		//	for (int i = 0; i < packet.FHead.DataSize; i++)
		//	{
		//		printf("%02X ", packet.Data[i]);
		//		outHeader << std::hex << static_cast<int>(packet.Data[i]) << " ";
		//	}

		//	outHeader << std::endl;

		//	seqNr++;
		//}

		//outHeader.close();
		//printf("First header is %d\n", _size);
	//}

	//Reset number of recv packets etc.
	int written = 0;
	int seqNr = m_eye[_eye].SeqNr;
	int maxPktSize = 0;
	long long timeAtComplete = Timers::Get().Time();

	Timers::Get().PushLongLong(_eye, T_DEC_BEGIN, readPosRaw, timeAtComplete); //decode begin
	Timers::Get().PushLongLong(_eye, T_NET_FIN, readPosRaw, timeAtComplete); //network finish

	//if (Timers::Get().DebugTest(_eye))
	//{
	//	m_framePartGraphMutex.lock();
	//	m_framePartsGraph << (Timers::Get().Time() - m_beginTime) << "				" << "slow" << "\n";
	//	m_framePartGraphMutex.unlock();
	//}

	//All packets received for this frame, here goes the unpacking to raw video data
	while(seqNr < PACKET_BUFFER_SIZE && !m_quit)
	{
		//Timers::Get().DeadlockCheck(startTime, "Client packet copy loop");

		//Get packet from queue
		m_eye[_eye].QueueMutex.lock();
		Pkt::Sv_FramePart& packet = *(Pkt::Sv_FramePart*)m_eye[_eye].Queue[read][seqNr];
		m_eye[_eye].QueueMutex.unlock();

		//Headers: Eth2 + IPv4 + UDP + FramePart + Data
		static const int HEADERS_SIZE = 14 + 20 + 8 + sizeof(Pkt::Sv_FramePartHeader);

		if (packet.FHead.DataSize + HEADERS_SIZE > maxPktSize)
		{
			maxPktSize = packet.FHead.DataSize + HEADERS_SIZE;
		}

		//Should never happen
		if (readPosRaw != packet.TimeStamp)
		{
			printf("%d out of sync! <read: %u stamp: %u pkt seq: %u seq: %d write: %u>\n",_eye, readPosRaw, packet.TimeStamp, packet.SequenceNr, seqNr, m_eye[_eye].RawPosW.load());
			readPosRaw = packet.TimeStamp;
			m_eye[_eye].RawPosR = packet.TimeStamp;
			m_eye[_eye].ReadFromPkt = 0;
			system("pause");
		}

		//The decoder requests _size bytes, this code takes care of a case where more data is available. 
		//But that should never happen nowadays though, it could happen when there was a demuxer here
		if (written + packet.FHead.DataSize > _size)
		{
			//Copy
			int remaining = _size - written;
			memcpy(_mem + written, packet.Data + m_eye[_eye].ReadFromPkt, remaining - m_eye[_eye].ReadFromPkt);
			written += remaining - m_eye[_eye].ReadFromPkt;
			//Save where we are reading in this specific packet
			m_eye[_eye].ReadFromPkt += remaining;

			break;
		}

		//Copy from packet to decoder memory
		memcpy(_mem + written, packet.Data + m_eye[_eye].ReadFromPkt, packet.FHead.DataSize - m_eye[_eye].ReadFromPkt);
		written += packet.FHead.DataSize - m_eye[_eye].ReadFromPkt;

		//No remainder
		m_eye[_eye].ReadFromPkt = 0;

		//Clear for next sweep
		m_eye[_eye].QueueMutex.lock();
		m_eye[_eye].Queue[read][seqNr] = nullptr;
		m_eye[_eye].QueueMutex.unlock();

		//Is final packet of this frame
		if (packet.IsLastPkt == 1)
		{
			//Print a bunch of statistics
			int printIndex = readPosRaw;

			//Timers::Get().PushLongLong(_eye, T_DEC_BEGIN, read, , false);
			Timers::Get().PushLongLong(_eye, T_PACKETS, printIndex, seqNr+1); //network finish

			if (_eye == 0)
			{
				Timers::Get().PushLongLong(_eye, T_INPUT_REDUNDANT, printIndex, m_inputPacket.SequenceNr - packet.Misc); //input
				Timers::Get().PushLongLong(_eye, T_INPUT_RESEND, printIndex, packet.Misc);
			}

			//Timers::Get().PushLongLong(_eye, T_JITTER, printIndex, syncWait);	//sync wait

			DumpStats(_eye, printIndex, packet, reSendRequests, timeAtComplete -jitterWait, maxPktSize, packet.FHead.Bitrate);

			//Increment read index
			m_eye[_eye].RawPosR.fetch_add(1, std::memory_order::memory_order_release);

			seqNr = 0;

			//Is a new bitrate set?
			if (m_bitrate != packet.FHead.Bitrate)
				printf("Bitrate is %d\n", packet.FHead.Bitrate);
			m_bitrate = packet.FHead.Bitrate;

			//Copy is complete
			break;
		}

		seqNr++;

		//Should not happen
		if (written == _size)
		{
			printf("Read all that decoder requested\n");
			break;
		}
	}

	if (seqNr != -1)
	{
		m_eye[_eye].SeqNr = seqNr;
	}
	else
	{
		written = -1;
		m_eye[_eye].SeqNr = 0;
	}

	return written;
}

bool Client::IsMorePacketsToDecode(int _eye)
{
	return (m_eye[_eye].SeqNr > 0);
}

void Client::DumpStats(unsigned char _eye, int read, const Pkt::Sv_FramePart& packet, int packetLoss, long long syncWait, int _pktSize, int _bitrate)
{

	Timers::Get().PushLongLong(_eye, T_PKT_SIZE, read, _pktSize);	//sync wait
	//Timers::Get().PushLongLong(_eye, T_BIT_RATE, read, m_lossChance);	//sync wait
	Timers::Get().PushLongLong(_eye, T_BIT_RATE, read, _bitrate);	//sync wait

	Timers::Get().PushLongLong(_eye, T_TIME_STAMP, read, packet.TimeStamp); //stamp
	
	Timers::Get().PushLongLong(_eye, T_IS_UDP, read, packet.IsUDP_IsAcked_IsRedundant); //UDP

}

//Not really used anymore except for disconnect packet
void Client::ListenTCP(int _channel)
{
	static short prevIndex[2] = { 0,0 };

	int index = m_packetBufferIndexTCP.fetch_add(1);

	char* buffer = (char*)m_packetBufferTCP[index % RAW_PACKET_BUFFER_SIZE_TCP];

	int received = Recv(m_socketTCP[_channel], buffer, sizeof(Pkt::RVR_Header)); //Read header type of 1bytes

	if (received > 0)
	{
		Pkt::RVR_Header header = *(Pkt::RVR_Header*)buffer;

		m_connectionProblem = 0;

		//Can use tcp for packets too, may be useful for comparison
		if (header.PayloadType == Pkt::SV_FRAME)
		{
			//Not used anymore

			received += Recv(m_socketTCP[_channel], buffer + sizeof(Pkt::RVR_Header), sizeof(Pkt::Sv_FramePartHeader)); //Read frame header

			Pkt::Sv_FramePartHeader* ptr = (Pkt::Sv_FramePartHeader*)(buffer + sizeof(Pkt::RVR_Header));

			received += Recv(m_socketTCP[_channel], buffer + sizeof(Pkt::RVR_Header) + sizeof(Pkt::Sv_FramePartHeader), ptr->DataSize); //Read frame data

			memcpy(m_packetBufferUDP[m_packetBufferIndexUDP.fetch_add(1) % RAW_PACKET_BUFFER_SIZE_UDP], buffer, received);

			std::lock_guard<std::mutex> guard(m_unpackMutex);
			m_packetIsFresh = true;
			m_unpackCondition.notify_one();
		}
		else if (header.PayloadType == Pkt::X_DISCONNECT)
		{
			m_connectionProblem = 1000;
			m_quit = true;
		}
	}
	else
	{
		int err = WSAGetLastError();
		if (err != WSAETIMEDOUT)
		{
			//printf("Connection problem %d %d\n", err, m_connectionProblem); //spam
			m_connectionProblem++;
		}
	}
}

void Client::StartThreads()
{
	if (m_threadList.size() == 0)
	{
		m_quit = 0;

		m_ThreadListenUDP = std::thread(ListenUDP_ThreadLoop, this);
		m_ThreadListenTCP[0] = std::thread(ListenTCP_ThreadLoop, this);
		m_ThreadListenTCP[1] = std::thread(ListenTCP_ThreadLoop, this);
		m_ThreadClockSync = std::thread(ClockSync_ThreadLoop, this);
		m_ThreadUDPunpacker = std::thread(UnpackUDP_ThreadLoop, this);

		m_threadList.push_back(&m_ThreadUDPunpacker);
		m_threadList.push_back(&m_ThreadListenUDP);
		m_threadList.push_back(&m_ThreadClockSync);
		m_threadList.push_back(&m_ThreadListenTCP[0]);
		m_threadList.push_back(&m_ThreadListenTCP[1]);

		//2 tcp threads are obviously not necessary, but is useful if want to test tcp for video in the future
	}
}

//This thread just handles ping and time calculation
//Should ideally be moved to be a function of input packets instead, since that sends and acks packets all the time anyway
int Client::ClockSync_ThreadLoop(void* ptr)
{
	Client* self = (Client*)ptr;

	long long lastClockSync = 0;

	while (!self->m_quit)
	{
		long long timeNow = Timers::Get().Time();

		if (timeNow - lastClockSync > 1000000)
		{
			self->ClockSync();
			lastClockSync = timeNow;
		}

		self->EnsureRetransmission();
	}

	return 1;
}

void Client::ClockSync()
{
	Pkt::X_ClockSync pkt;
	pkt.ClientTime = Timers::Get().Time();
	pkt.LastPing = Timers::Get().Ping();
	pkt.LastServerDiff = Timers::Get().ServerDiff();

	MessageUDP(sizeof(Pkt::X_ClockSync), &pkt);

}

int Client::UnpackUDP_ThreadLoop(void* ptr)
{
	Client* self = (Client*)ptr;

	while (!self->m_quit)
	{
		self->UnpackUDPpkt();
	}

	return 1;
}

void Client::UnpackUDPpkt()
{
	//while(!m_quit)
	{
		std::unique_lock<std::mutex> waitLock(m_unpackMutex);
		if (m_unpackCondition.wait_for(waitLock, std::chrono::microseconds(1), [this] { return m_packetIsFresh; }))
		{
			m_packetIsFresh = false;
		}
	}

	if (m_quit)
		return;

	unsigned int index = 0;
	unsigned int myIndex = 0;

	int busyWaits = 0;

	//m_framePartGraphMutex.lock();
	//m_framePartsGraph << (Timers::Get().Time() - m_beginTime) << "					" << 1 << "\n";
	//m_framePartGraphMutex.unlock();

	while(!m_quit)
	{
		index = m_packetBufferIndexUDP.load();
		myIndex = m_unpackIndexUDP.load();

		if (myIndex >= index)
		{
			break;
			/*busyWaits++;

			if (busyWaits > 10000)
			{
				m_framePartGraphMutex.lock();
				m_framePartsGraph << (Timers::Get().Time() - m_beginTime) << "					" << 0 << "\n";
				m_framePartGraphMutex.unlock();

				break;
			}
			else
				continue;*/
		}

		char* buffer = (char*)m_packetBufferUDP[myIndex % RAW_PACKET_BUFFER_SIZE_UDP];

		m_unpackIndexUDP++;

		if (buffer[0] == -111)
			continue;

		Pkt::RVR_Header* header = (Pkt::RVR_Header*)buffer;

		//static int recvPkts = 0;

		//recvPkts++;

		//printf("recv by client %d			\r", recvPkts);
		if (header->PayloadType == Pkt::X_DUMMY)
		{

		}
		else if (header->PayloadType == Pkt::SV_FRAME)
		{
			Pkt::Sv_FramePart& packet = *(Pkt::Sv_FramePart*)buffer;
			packet.IsUDP_IsAcked_IsRedundant = 1;
			m_serverIsUsingUDP = true; //This just enables tcp to be tested if needed
			int received = packet.FHead.DataSize + sizeof(Pkt::Sv_FramePartHeader) + sizeof(Pkt::RVR_Header);
			RecvSvFramePart(packet, received);

			//In case ack for input was lost, this will ack too
			if (packet.FHead.InputIndexAck > m_inputPacketAck)
			{
				OnAckedInputPkt(packet.FHead.InputIndexAck);
			}

			//Stats
#ifdef PRINT_FRAME_PARTS
			m_framePartGraphMutex.lock();
			if(packet.FHead.Resends == 0)
				m_framePartsGraph << (Timers::Get().Time() - m_beginTime) << "	" << packet.SequenceNr << "	" << packet.TimeStamp << "\n";
			else
				m_framePartsGraph << (Timers::Get().Time() - m_beginTime) << "		" << packet.TimeStamp <<  "	" << packet.SequenceNr << "\n";
			m_framePartGraphMutex.unlock();
#endif
		}
		else if (header->PayloadType == Pkt::X_CLOCK_SYNC)
		{
			//Ping and time diff

			Pkt::X_ClockSync& packet = *(Pkt::X_ClockSync*)buffer;

			m_ping = Timers::Get().Time() - packet.ClientTime;
			long long diff = packet.ServerTime - packet.ClientTime - (m_ping / 2);

			//printf("Ping: %lld | Diff: %lld | Server: %lld | Client: %lld\n", ping, diff,packet.ServerTime,packet.ClientTime);

			Timers::Get().SetPing(m_ping);
			Timers::Get().SetServerTimeDiff(diff);
		}
		else if (header->PayloadType == Pkt::SV_ACK)
		{
			Pkt::Sv_Ack& packet = *(Pkt::Sv_Ack*)buffer;

			if (packet.InputPktsRedundant > m_redundantInputPktsTotal)
			{
				m_redundantInputPktsTotal = packet.InputPktsRedundant;
			}

			//Server acking an input packet
			if (packet.InputPktTimestamp >= m_inputPacketAck || m_inputPacketAck == -2)
			{
				OnAckedInputPkt(packet.InputPktTimestamp);
			}
			else
			{
				printf("Multiple input acks received %u %lld %lld\n", packet.InputPktTimestamp, m_inputIndex, m_inputPacketAck);
			}
		}
		else if (header->PayloadType == Pkt::X_PERFORMANCE)
		{
			//Not used anymore
		}
		
	}
}

void Client::OnAckedInputPkt(unsigned int packetIndex)
{
	//printf("Acked input %d\n", packetIndex);

	m_inputPacketAck = packetIndex;
	m_inputPktACKTimeConsumed = Timers::Get().Time() - m_lastInputPktSentTime;

	long long inputTime = 
		Timers::Get().PullLongLong(0, m_ACKpkt.TimeStamp, T_RENDER_BEGIN) -
		Timers::Get().PullLongLong(0, m_ACKpkt.TimeStamp, T_INPUT_BEGIN) -
		Timers::Get().GetServerDiff();

	//No loss, sample is added to future gamma calculations
	m_inputResendPktDelay.AddSample(m_inputPktACKTimeConsumed, m_ping, true, inputTime);

	std::lock_guard<std::mutex> guard(m_runMutex);
	m_inputIsFresh = true;
	m_inputCondition.notify_one();
}

void Client::Update()
{
	if (m_connectionProblem > 0)
	{
		printf("Disconnecting from server...\n");
		Disconnect();
		m_connectionProblem = 0;
		printf("Disconnecting complete\n");
	}
}

int Client::GetCurrentFrameID(int _eye)
{
	return m_eye[_eye].RawPosR.load(std::memory_order::memory_order_acquire);
}

void Client::SendInputMatrix(bool _forceIDR, float _dt, int _predict)
{

	if (!_forceIDR)
	{
		//UpdateHMDMatrixPose is vsync, contains WaitGetPoses() which is blocking call
		m_hmdPose = m_vrInputs->UpdateHMDMatrixPose(_dt, _predict);
	}

	m_inputPacket.SequenceNr = 0; //Resends

	m_inputPacket.PiggybackACK = m_ACKpkt.TimeStamp;
	m_inputPacket.ClockTime = Timers::Get().Time();
	m_inputPacket.TimeStamp = (unsigned int)++m_inputIndex;
	m_inputPacket.InputType = Pkt::InputTypes::MATRIX_HMD_POSE;
	memcpy(m_inputPacket.Data, &m_hmdPose, sizeof(glm::mat4));

	SendInputPacket();

	//Hack, extra input goes here
	m_inputPacket.DebugTimers[4] = 0;

	std::lock_guard<std::mutex> guard(m_ensureTransmissionMutex);
	m_ensureTransmissionNow = true;
	m_ensureTransmissionCond.notify_one();

	//Makes sure the udp input pkt arrives, comment out for tcp
	//EnsureRetransmission(); 


}

//This is for input that must happen immediately in the next frame for experiment accuracy
//In hindsight, perhaps all input should go here
void Client::SetKeyPress(int _key)
{
	m_inputPacket.Misc = _key; 
}

//Hack that enables two immediate inputs for experiments
void Client::SetKeyPressExtraHack(int _key)
{
	m_inputPacket.DebugTimers[4] = _key;
}

void Client::SendInputPacket()
{
	m_sentInputPktsTotal++;

	MessageUDP(sizeof(Pkt::Cl_Input), &m_inputPacket);

	m_lastInputPktSentTime = Timers::Get().Time();

	// Buffer flush hack for scrappy 60Ghz dongle

		//static Pkt::X_Dummy dummy;

		//for (int i = 0; i < 3; i++)
		//{
		//	MessageUDP(sizeof(Pkt::X_Dummy), &dummy);
		//}

	//If testing with TCP

		//MessageTCP(sizeof(Pkt::Cl_Input), &m_inputPacket, 0);
}

//Ensures input packets arrive by using re-transmission
void Client::EnsureRetransmission()
{
	{
		std::unique_lock<std::mutex> waitLock(m_ensureTransmissionMutex);
		if (m_ensureTransmissionCond.wait_for(waitLock, std::chrono::microseconds(1000), [this] { return m_ensureTransmissionNow; }))
		{
			m_ensureTransmissionNow = false;
		}
	}

	if (m_inputPacketAck >= m_inputIndex)
	{
		return;
	}

	long long lastReqTime = Timers::Get().Time();
	long long t0 = lastReqTime;
	int resends = 0;

	//if (m_lossInPrevInput)
	//{
	//	//If the frame had an input loss, dont add the sample to gamma distribution
	//	m_lossInPrevInput = 0;
	//}
	//else 
	//{
	//	long long inputTime = Timers::Get().PullLongLong(0, m_ACKpkt.TimeStamp - 1, T_RENDER_BEGIN) - Timers::Get().PullLongLong(0, m_ACKpkt.TimeStamp - 1, T_INPUT_BEGIN) - Timers::Get().GetServerDiff();

	//	//No loss, sample is added to future gamma calculations
	//	m_inputResendPktDelay.AddSample(m_inputPktACKTimeConsumed, m_ping, true, inputTime);
	//}

	//Re-transmission logic
	while (!m_quit)
	{
		if (m_inputPacketAck >= m_inputIndex)
		{
			break;
		}

		long long timeSinceReq = Timers::Get().Time() - lastReqTime;

		//Check gamma distribution if this is a long wait
		if (m_inputResendPktDelay.IsSignificantWaitGamma(timeSinceReq))
		{
			m_inputPacket.SequenceNr++; //Tells server how many resends this input packet has
			SendInputPacket();
			lastReqTime = Timers::Get().Time();

			m_resentInputPktsTotal++;
			resends++;

			//Busy wait, but we don't need the processor anyway, waiting for server at this state.
			//Conditional wait should work fast though, but I commented this out at some point and would need to test again to make sure.
			//{
			//	std::unique_lock<std::mutex> waitLock(m_runMutex);
			//	if (m_inputCondition.wait_for(waitLock, std::chrono::microseconds(1), [this] { return m_inputIsFresh; }))
			//	{
			//		m_inputIsFresh = false;
			//	}
			//}
		}

		//This will spin as fast as possible now without cond wait..
	}

	//How long it took to get an ack for this input pkt
	//m_inputPktACKTimeConsumed = Timers::Get().Time() - t0;

	//Can print the number of re-transmissions in the delay graph if needed
	//m_inputResendPktDelay.AddDebugValue(resends);
}