#pragma once

#define UDP_PORT 17112
#define TCP_PORT 17113

#define FRAME_PAYLOAD_SIZE 1401

#include <bitset>
#include <VertexDataLens.h>

namespace Pkt
{
	enum PacketEnum : unsigned char
	{
		X_DISCONNECT,
		CL_QOS,
		CL_INPUT,
		X_NACK_VECTOR,
		SV_FRAME,
		X_CLOCK_SYNC,
		CL_LOSS,
		X_ACK,
		SV_ACK,
		X_NACK,
		X_PERFORMANCE,
		CL_LENS_DATA,
		SV_INPUT_FEEDBACK,
		CL_KEY_PRESS,
		X_DUMMY,
		CL_HIDDEN_AREA_MESH,

		UNKNOWN_PACKET,
	};

	enum InputTypes : unsigned char
	{
		MATRIX_HMD_POSE,
		MATRIX_EYE_POS_LEFT,
		MATRIX_EYE_POS_RIGHT,
		MATRIX_EYE_PROJ_LEFT,
		MATRIX_EYE_PROJ_RIGHT,
		VECTOR_RESOLUTION_0,
		VECTOR_RESOLUTION_1,
		VECTOR_RESOLUTION_2,
		VECTOR_RESOLUTION_3,
		VECTOR_RESOLUTION_4,
	};

	static bool IsMatrix(InputTypes _type)
	{
		return _type >= MATRIX_HMD_POSE && _type <= MATRIX_EYE_PROJ_RIGHT;
	}

	static bool IsVector(InputTypes _type)
	{
		return _type >= VECTOR_RESOLUTION_0 && _type <= VECTOR_RESOLUTION_4;
	}

	//union QoSData
	//{
	//	unsigned int InputIndex;
	//	unsigned int PacketsLost;
	//	bool Ready;
	//	unsigned int Coordinate;
	//	unsigned int Level;
	//};

#pragma pack(push, 1)

	struct RVR_Header 
	{
		unsigned char PayloadType : 6;       /* payload type */
		unsigned char IsUDP_IsAcked_IsRedundant : 1;
		unsigned char IsLastPkt : 1;        /* marker bit */ //Indicates re-transmission in Cl_Input
		unsigned char Misc = 0;
		unsigned short SequenceNr = 0;        /* sequence number */
		unsigned int TimeStamp = 0;           /* timestamp */
		long long DebugTimers[5] = { 0 };


		RVR_Header(bool _isLastPacket, PacketEnum _type, unsigned short _packetNr,unsigned int _index, unsigned int _sourceID)
		{
			IsLastPkt = _isLastPacket;
			PayloadType = _type;
			SequenceNr = _packetNr;
			TimeStamp = _index;
		}

		RVR_Header(PacketEnum _type)
		{
			IsLastPkt = 0;
			PayloadType = _type;
			SequenceNr = 0;
			TimeStamp = 0;
		}

		RVR_Header()
		{
			IsLastPkt = 0;
			PayloadType = UNKNOWN_PACKET;
			SequenceNr = 0;
			TimeStamp = 0;
		}
	};

	//struct Header
	//{
	//	PacketEnum Head = UNKNOWN_PACKET;
	//	Header(PacketEnum _header): Head(_header)
	//	{

	//	}
	//	Header() : Head(UNKNOWN_PACKET)
	//	{

	//	}
	//};

	struct X_Dummy :RVR_Header
	{
		char dummy[1024];
		X_Dummy():RVR_Header(PacketEnum::X_DUMMY) { }
	};

	struct X_Disconnect:RVR_Header
	{
		X_Disconnect():RVR_Header(PacketEnum::X_DISCONNECT) { }
	};

	struct Cl_HiddenAreaMesh :RVR_Header
	{
		Cl_HiddenAreaMesh() :RVR_Header(PacketEnum::CL_HIDDEN_AREA_MESH) { }
	};

	//struct Cl_QoS :RVR_Header
	//{
	//	QoSEnum Type;
	//	QoSData Data1;
	//	QoSData Data2;
	//	Cl_QoS() :RVR_Header(PacketEnum::CL_QOS) { }
	//};

	struct Cl_Input :RVR_Header
	{
		InputTypes InputType = MATRIX_HMD_POSE;
		unsigned int PiggybackACK = 0;
		long long ClockTime=0;
		unsigned char Data[128];
		//unsigned char Dummy[1024];

		Cl_Input() :
			RVR_Header(PacketEnum::CL_INPUT) { }
	};

	struct Cl_LensData :RVR_Header
	{
		VertexDataLens Data[64];
		Cl_LensData() :
			RVR_Header(PacketEnum::CL_LENS_DATA) { }
	};

	struct X_NackVector :RVR_Header
	{
		unsigned char Eye=0;
		std::bitset<512> AckVector;

		X_NackVector() :
			RVR_Header(PacketEnum::X_NACK_VECTOR) { }
	};

	struct X_Ack :RVR_Header
	{
		X_Ack() :
			RVR_Header(PacketEnum::X_ACK) { }
	};

	struct Cl_KeyPress :RVR_Header
	{
		Cl_KeyPress() :
			RVR_Header(PacketEnum::CL_KEY_PRESS) { }
	};

	struct Sv_Ack :RVR_Header
	{
		//Only for debugging input packet losses
		unsigned int InputPktTimestamp = 0;
		unsigned int InputPktsRedundant = 0;

		Sv_Ack() :
			RVR_Header(PacketEnum::SV_ACK) { }
	};

	struct X_Nack :RVR_Header
	{
		//unsigned char Eye = 0;
		//long long OriginalTime = 0;
	
		X_Nack() :
			RVR_Header(PacketEnum::X_NACK) { }
	};

	struct X_Performance :RVR_Header
	{
		//Sequence number is the number of redundant packets received.

		X_Performance() :
			RVR_Header(PacketEnum::X_PERFORMANCE) { }
	};

	struct Sv_FramePartHeader
	{
		unsigned char Level : 3;
		unsigned char Eye : 1;
		unsigned char Resends : 4;
		unsigned short DataSize = 0;
		unsigned short Bitrate = 0;
		unsigned int InputIndexAck = 0;
	};

	struct Sv_FramePart :RVR_Header
	{
		Sv_FramePartHeader FHead;
		unsigned char Data[FRAME_PAYLOAD_SIZE];

		Sv_FramePart() :
			RVR_Header(PacketEnum::SV_FRAME) 
		{
			FHead.Level = 0;
			FHead.Eye = 0;
			FHead.Resends = 0;
			FHead.DataSize = 0;
			FHead.InputIndexAck = 0;
			memset(Data, 0, FRAME_PAYLOAD_SIZE);
		}
	};

	struct X_ClockSync :RVR_Header
	{
		long long ClientTime=0;
		long long ServerTime=0;
		long long LastPing=0;
		long long LastServerDiff=0;

		X_ClockSync(bool _isLastPacket, unsigned short _packetNr, unsigned int _index, unsigned int _sourceID) :
			RVR_Header(_isLastPacket, PacketEnum::X_CLOCK_SYNC, _packetNr, _index, _sourceID) { }

		X_ClockSync() :
			RVR_Header(PacketEnum::X_CLOCK_SYNC) { }
	};

	struct X_Report :RVR_Header
	{
		//Marker 0 == Ack
		//Marker 1 == Loss

		X_Report(bool _isLastPacket, unsigned short _packetNr, unsigned int _index, unsigned int _sourceID) :
			RVR_Header(_isLastPacket, PacketEnum::CL_LOSS, _packetNr, _index, _sourceID) { }

		X_Report() :
			RVR_Header(PacketEnum::CL_LOSS) { }
	};

	
#pragma pack(pop)
}
