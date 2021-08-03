/*
* 
*/

#pragma once

#include <NetBase.h>
#include <unordered_map>
#include <atomic>
#include <queue>
#include <mutex>
#include <fstream>
#include "../Core/VRInputs.h"
#include <StatCalc.h>

class Client:public NetBase
{
public:
	Client();
	~Client();

	int MessageUDP(int _size, void * _data);

	void ListenUDP();

	int CopyFBBuffer(unsigned char* _mem, unsigned char _eye, int size);

	bool IsMorePacketsToDecode(int _eye);

	void DumpStats(unsigned char _eye, int read, const Pkt::Sv_FramePart& packet, int packetLoss, long long syncWait, int _pktSize, int _bitrate);

	void ListenTCP(int _channel);

	void StartThreads();

	void Update();

	bool IsStopped() { return m_quit; }

	int GetCurrentFrameID(int _eye);

	void SendInputMatrix(bool _forceIDR, float _dt, int _predict);
	void SetKeyPress(int _level);

	void SetKeyPressExtraHack(int _key);
	void SendInputPacket();

	int GetCurrentLevel() { return m_level; }

	void SetVRInput(VRInputs* _vrInputs) { m_vrInputs = _vrInputs; }

	int GetBitRate() { return m_bitrate; };
	void EnsureRetransmission();
private:

	void RecvSvFramePart(Pkt::Sv_FramePart & packet, int _dataSize);



	static int ClockSync_ThreadLoop(void* ptr);
	static int UnpackUDP_ThreadLoop(void* ptr);

	void UnpackUDPpkt();

	void OnAckedInputPkt(unsigned int packetIndex);

	void ClockSync();

	std::thread m_ThreadListenUDP;
	std::thread m_ThreadListenTCP[2];
	std::thread m_ThreadClockSync;
	std::thread m_ThreadUDPunpacker;

	static const int LARGEST_PACKET_SIZE = sizeof(Pkt::Sv_FramePart)+3; //why +3 ?

	//This certainly does not need to be this big, but whatever
	static const int RAW_PACKET_BUFFER_SIZE_UDP = 32000;
	static const int RAW_PACKET_BUFFER_SIZE_TCP = 1024;
	std::atomic<unsigned int> m_packetBufferIndexUDP = 0;
	std::atomic<unsigned int> m_packetBufferIndexTCP = 0;
	std::atomic<unsigned int> m_unpackIndexUDP = 0;

	char m_packetBufferUDP[RAW_PACKET_BUFFER_SIZE_UDP][LARGEST_PACKET_SIZE];
	char m_packetBufferTCP[RAW_PACKET_BUFFER_SIZE_TCP][LARGEST_PACKET_SIZE];

	struct ParallelEyeData
	{
		std::atomic<unsigned int> PacketsRecvdThisFrame;
		unsigned short PacketsNeededThisFrame;
		volatile long long FirstPktRecvTime;
	};

	struct EyeData
	{
		std::atomic<unsigned int> RawPosW = 0;
		std::atomic<unsigned int> RawPosR = 0;

		static const int EYE_DATA_MEMORY_SLOTS = 8;

		ParallelEyeData ParallelData[EYE_DATA_MEMORY_SLOTS];

		unsigned short SeqNr = 0;
		volatile Pkt::Sv_FramePart* Queue[FRAME_BUFFER_SIZE][PACKET_BUFFER_SIZE]; //How many packets are needed max for one frame? a lot less than 512 but ok.
		std::mutex QueueMutex;
		int ReadFromPkt = 0;

		std::mutex BandwidthMutex;

		bool IsFresh = false;
		std::mutex RunMutex;
		std::condition_variable Condition;
		//volatile unsigned int FrameID = 0;

		//Enables wake up when all data is received
		void EyeData::CondWait(int _microseconds)
		{
			std::unique_lock<std::mutex> waitLock(RunMutex);
			if (Condition.wait_for(waitLock, std::chrono::microseconds(_microseconds), [this] { return IsFresh; }))
			{
				IsFresh = false;
			}
		}

		//Wakes up
		void EyeData::GoDecoder()
		{
			std::lock_guard<std::mutex> guard(RunMutex);
			IsFresh = true;
			Condition.notify_one();
		}

		void EyeData::Reset(int _index)
		{
			ParallelData[_index].PacketsNeededThisFrame = 512; //To reset
			ParallelData[_index].PacketsRecvdThisFrame = 0;
		}

		bool EyeData::IsAllPacketsReceivedThisFrame(int _index)
		{
			return (ParallelData[_index].PacketsRecvdThisFrame.load() >= ParallelData[_index].PacketsNeededThisFrame);
		}
	};

	EyeData m_eye[2];

	int m_level = 0;

	glm::mat4 m_hmdPose;

	VRInputs* m_vrInputs=nullptr;

	bool m_serverIsUsingUDP = false;

	unsigned int m_redundantPackets = 0;
	long long m_inputIndex = -1;

	volatile long long m_inputPacketAck = -2;

	Pkt::Cl_Input m_inputPacket;

	//Gamma distribution stuff
	StatCalc m_statsResendPktDelay = StatCalc("VideoPktDelay");
	StatCalc m_inputResendPktDelay = StatCalc("InputPktACKDelay");

	bool m_inputIsFresh = false;
	std::mutex m_runMutex;
	std::condition_variable m_inputCondition;

	bool m_packetIsFresh = false;
	std::mutex m_unpackMutex;
	std::condition_variable m_unpackCondition;

	bool m_ensureTransmissionNow = false;
	std::mutex m_ensureTransmissionMutex;
	std::condition_variable m_ensureTransmissionCond;

	Pkt::X_Ack m_ACKpkt;

	std::ofstream m_framePartsGraph;
	std::mutex m_framePartGraphMutex;

	static long long m_beginTime;

	unsigned int m_sentInputPktsTotal = 0;
	std::atomic<unsigned int> m_redundantInputPktsTotal = 0;
	unsigned int m_resentInputPktsTotal = 0;

	long long m_inputPktACKTimeConsumed = 0;
	long long m_lastInputPktSentTime = 0;

	int m_lossInPrevInput = false;
	int m_bitrate = 0;
};