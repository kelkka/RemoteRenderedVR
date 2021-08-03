#include "NetworkEngine.h"
#include "../Network/Server.h"
#include "Encode/EncoderBase.h"
#include "../Utility/Timers.h"

NetworkEngine::NetworkEngine(Server * _server, EncoderBase* _encoder)
{
	//m_client = _client;
	m_maxPktSize = FRAME_PAYLOAD_SIZE;
	m_server = _server;
	m_encoder = _encoder;
	m_remainingTCPFrames = 0;
	std::thread thread(ThreadStart, this);
	thread.detach();
}

NetworkEngine::~NetworkEngine()
{
}

void NetworkEngine::ThreadStart(NetworkEngine* _self)
{
	_self->RunProcess();
}

void NetworkEngine::SpecificProcess()
{
	long long timeNow = Timers::Get().Time();

	EncodedFrame& encodedFrame = m_encoder->GetMemoryPtr()->Peek();

	Pkt::Sv_FramePart framePart;
	framePart.TimeStamp = encodedFrame.Index;
	framePart.FHead.Eye = encodedFrame.Eye;
	framePart.FHead.Level = encodedFrame.Level;
	framePart.SequenceNr = -1;
	framePart.FHead.Bitrate = m_encoder->GetBitrate() / 1000; //kbit
	framePart.FHead.InputIndexAck = m_server->GetLastAckedInput();
	framePart.Misc = (unsigned char)m_server->GetInputLossThisFrameReset();
	//if (framePart.FHead.InputIndexAck == lastInputpkt)
	//{
	//	printf("wtf\n");
	//}

	//printf("Sending with input %d\n", framePart.FHead.InputIndexAck);
	//if(encodedFrame.Eye == 0)
	//	framePart.Misc = m_server->GetRedundantInputPkts();
	//lastInputpkt = framePart.FHead.InputIndexAck;
	memcpy(framePart.DebugTimers, encodedFrame.TimeStamps, sizeof(long long) * 4);

	bool forceTCP = false;

	//if (m_previousLevel != framePart.FHead.Level)
	//{
	//	m_remainingTCPFrames = 5;
	//}

	if (m_remainingTCPFrames > 0)
	{
		forceTCP = true;
		m_remainingTCPFrames--;
	}

	//forceTCP = true;

	//PACKET SIZE TEST

	//if (m_maxPktSize > 512 && forceTCP == false)
	//{
	//	m_maxPktCounter++;
	//	if (m_maxPktCounter > 5)
	//	{
	//		m_maxPktSize--;
	//		printf("Packet size is now %d\n", m_maxPktSize);
	//		m_maxPktCounter = 0;
	//	}
	//}

	int payloadSize = m_maxPktSize;

	m_previousLevel = framePart.FHead.Level;
	bool hasMarker = 0;
	unsigned short packetSize = 0;
	int endPkt = (encodedFrame.Size-1) / payloadSize;

	for (int i = 0; i < encodedFrame.Size; i += payloadSize)
	{
		int copySize = payloadSize;
		framePart.SequenceNr++;

		if (i >= encodedFrame.Size - payloadSize)
		{
			copySize = encodedFrame.Size - i;
			framePart.IsLastPkt = 1;
			hasMarker = 1;
		}

		if (copySize == 0)
		{
			printf("Server attempt to send zero size %d %d\n", encodedFrame.Size, i);
			continue;
		}

		if (framePart.SequenceNr >= NetBase::PACKET_BUFFER_SIZE)
		{
			printf("Frame consisted of more than %d packets!!!!\n", NetBase::PACKET_BUFFER_SIZE);
			system("pause");
		}

		framePart.FHead.DataSize = copySize;
		memcpy(framePart.Data, encodedFrame.Memory + i, copySize);

		packetSize = sizeof(Pkt::RVR_Header) + sizeof(Pkt::Sv_FramePartHeader) + framePart.FHead.DataSize;

		framePart.DebugTimers[4] = Timers::Get().Time();

		//printf("Sending %d %d %d (%d)\r", framePart.FHead.Eye, framePart.TimeStamp, framePart.SequenceNr, framePart.Marker);

		if(forceTCP)
			m_server->MessageTCP(packetSize, &framePart, framePart.FHead.Eye);
		else
			m_server->MessageUDP(packetSize, &framePart);

		m_server->PushSendAgainBuffer(framePart.FHead.Eye, framePart.TimeStamp, framePart.SequenceNr, &framePart);

		/*if (framePart.IsLastPkt)
		{
			for (int i = 0; i < 2; i++)
			{
				static Pkt::X_Dummy dummy;
				m_server->MessageUDP(sizeof(Pkt::X_Dummy), &dummy);
			}
		}*/
	}

	//for (int i = 0; i < 3; i++)
	//{
	//	static Pkt::X_Dummy dummy;
	//	m_server->MessageUDP(sizeof(Pkt::X_Dummy), &dummy);
	//}

	if(forceTCP==false)
		m_server->EnsureRetransmissions(framePart.FHead.Eye, framePart.TimeStamp, timeNow);
	//
	//printf("Completed send %d %d\n", framePart.FHead.Eye, framePart.TimeStamp);
	//while (!m_quit && !forceTCP)
	//{
	//	std::this_thread::sleep_for(std::chrono::microseconds(1500));

	//	if (m_server->IsFrameAcked(framePart.TimeStamp))
	//		break;

	//	printf("Retransmitting marker frame! %u \n", framePart.TimeStamp);
	//	m_server->MessageUDP(packetSize, &framePart);
	//}

	//printf("Packets sent per frame %d\n", framePart.SequenceNr);
}