/*
* 
*/

#include "Timers.h"
#include <glm.hpp>
#include <algorithm>
#include <omp.h>

Timers& Timers::Get()
{
	static Timers timers = Timers();
	return timers;
}

void Timers::Init(int _width, int _height)
{
	m_fileDebugOutput = new std::ofstream("../content/timers1.txt", std::ofstream::out);

	*m_fileDebugOutput << "Step	Raw_Tot	UDP	Timestamp	Video-Resend	Video-Redundant	JitterWait	Game_Render	Encode	Network_Video	Decode	Display	Total	ClockDiff	Ping	FPSx90	RESx(";
	*m_fileDebugOutput << _width << "x" << _height << ")	";
	*m_fileDebugOutput << "Network_Input	kbps	Bytes	Pkt Size	Bitrate	Input-Resend	Input-Redundant	Packets	Drops\n";
}

long long Timers::Time()
{
	//auto timept = std::chrono::high_resolution_clock::now();
	//auto now_us = std::chrono::time_point_cast<std::chrono::microseconds>(timept);

	long long timenow = long long(omp_get_wtime() * 1000000);

	return timenow;// now_us.time_since_epoch().count();
}

void Timers::SetPing(long long _ping)
{
	m_ping = _ping;
}

void Timers::SetServerTimeDiff(long long _diff)
{
	if (m_diffs.size() < TIMER_ENTRIES)
	{
		m_diffs.push_back(_diff);
	}
	else
	{
		m_diffs[m_diffIndex % TIMER_ENTRIES] = _diff;
		m_diffIndex++;
	}

	size_t n = m_diffs.size() / 2;
	std::nth_element(m_diffs.begin(), m_diffs.begin() + n, m_diffs.end());
	m_serverDiff = m_diffs[n];
}

long long Timers::GetServerDiff()
{
	return m_serverDiff;
}

bool Timers::DeadlockCheck(long long _t0, const char* _str)
{
	if (Time() - _t0 > 10000000)
	{
		printf("Deadlock! %s\n", _str);
		return true;
	}
	return false;
}

Timers::Timers()
{
}

Timers::~Timers()
{
	Destroy();
}

void Timers::Destroy()
{
	for (int i = 0; i < PRINT_LAG-1; i++)
	{
		m_lastPrintedIndex++;
		DoPrint(m_lastPrintedIndex % TIMER_ENTRIES, m_lastPrintedIndex, 0);
	}



	if(m_fileDebugOutput->is_open())
		m_fileDebugOutput->close();
}

//void Timers::SetDeltaTime(float _dt)
//{
//	m_deltaTime = _dt;
//	if (m_deltaIndex >= DELTA_MAX_INDEX)
//		m_deltaIndex = 0;
//
//	m_pastDeltas[m_deltaIndex++] = _dt;
//
//	m_deltaTime = 0;
//	for (int i = 0; i < DELTA_MAX_INDEX; i++)
//	{
//		m_deltaTime += m_pastDeltas[i];
//	}
//	m_deltaTime /= DELTA_MAX_INDEX;
//}
//
//float Timers::GetDeltaTime()
//{
//	return m_deltaTime;
//}

void Timers::PushFloat(int _eye, int _identifier, int _frameIndex, double _val)
{
	if (_identifier >= T_MAX)
	{
		printf("Identifier OOB!\n");
		system("pause");
	}
	//_frameIndex = m_lastPrintedIndex;

	_frameIndex = _frameIndex % TIMER_ENTRIES;

	m_doubles[_eye][_frameIndex][_identifier] = _val;
}

void Timers::AddLongLong(int _eye, int _identifier, int _frameIndex, long long _val)
{
	if (_identifier >= T_MAX)
	{
		printf("Identifier OOB!\n");
		system("pause");
	}

	if (_val == 0)
		return;

	//_frameIndex = m_lastPrintedIndex;

	_frameIndex = _frameIndex % TIMER_ENTRIES;

	if (m_timers[_eye][_frameIndex][_identifier] == -1)
		m_timers[_eye][_frameIndex][_identifier] = 0;

	m_timers[_eye][_frameIndex][_identifier] += _val;
}

void Timers::PushLongLong(int _eye, int _identifier, int _id, long long _val)
{
	if (_identifier >= T_MAX)
	{
		printf("Identifier OOB!\n");
		system("pause");
	}
	//_id = m_lastPrintedIndex;

	//int actualIndex = _id;
	_id = _id % TIMER_ENTRIES;

	//if (T_DIS_FIN == _identifier)
	//{
	//	printf("T_DIS_FIN: %d %d %lld\n",_eye, _id, _val);
	//}

	m_timers[_eye][_id][_identifier] = _val;
}

void Timers::Print(int _id)
{
	m_lastPrintedIndex = _id;

	if (m_lastPrintedIndex > (PRINT_LAG-1))
	{
		int toPrint = m_lastPrintedIndex - PRINT_LAG;
		DoPrint(toPrint % TIMER_ENTRIES, toPrint, 0);
	}

	//m_lastPrintedIndex++;
}

void Timers::DoPrint(int _id, int actualIndex, int _eye)
{
	if (m_timers[_eye][_id][T_TIME_STAMP] == -1)
	{
		return;
	}

	static int prevFrame[2] = { 0,0 };

	if (HasStartedPrinting())
	{
		//if (m_timers[0][_id][T_DEC_FIN] == -1)
		//{
		//	int prevIndex = (_id + TIMER_ENTRIES - 1) % TIMER_ENTRIES;
		//	printf("Chaning index %d to %d || %lld to %lld\n", _id, prevIndex, m_timers[0][_id][T_DEC_FIN], m_timers[0][prevIndex][T_DEC_FIN]);
		//	m_timers[0][_id][T_DEC_FIN] = m_timers[0][prevIndex][T_DEC_FIN];
		//}

		//if (m_timers[1][_id][T_DEC_FIN] == -1)
		//{
		//	int prevIndex = (_id + TIMER_ENTRIES - 1) % TIMER_ENTRIES;
		//	printf("Chaning index %d to %d || %lld to %lld\n", _id, prevIndex, m_timers[1][_id][T_DEC_FIN], m_timers[1][prevIndex][T_DEC_FIN]);
		//	m_timers[1][_id][T_DEC_FIN] = m_timers[1][prevIndex][T_DEC_FIN];
		//}

		*m_fileDebugOutput << actualIndex - prevFrame[_eye] << "	";																	//Step
		*m_fileDebugOutput << m_doubles[_eye][_id][T_FPSx90] << "	";																	//Raw total time
		*m_fileDebugOutput << m_timers[_eye][_id][T_IS_UDP] << "	";																		//Is UDP
		*m_fileDebugOutput << m_timers[_eye][_id][T_TIME_STAMP] << "	";																	//Timestamp


		*m_fileDebugOutput << m_timers[_eye][_id][T_VIDEO_RE_SENDS] << "	";																//Resends
		*m_fileDebugOutput << m_timers[_eye][_id][T_VIDEO_REDUNDANT] << "	";																//Redundant resends

		*m_fileDebugOutput << m_timers[0][_id][T_JITTER] << "	";																//Jitter wait

		//*m_fileDebugOutput << glm::max(m_timers[0][_id][T_JITTER] - m_timers[0][_id][T_INPUT_BEGIN],
		//	m_timers[1][_id][T_JITTER] - m_timers[1][_id][T_INPUT_BEGIN]) << "	";																//Jitter wait

		*m_fileDebugOutput << glm::max(m_timers[0][_id][T_ENC_BEGIN] - m_timers[0][_id][T_RENDER_BEGIN],
			m_timers[1][_id][T_ENC_BEGIN] - m_timers[1][_id][T_RENDER_BEGIN]) << "	";							//Render

		*m_fileDebugOutput << glm::max(m_timers[0][_id][T_NET_BEGIN] - m_timers[0][_id][T_ENC_BEGIN],
			m_timers[1][_id][T_NET_BEGIN] - m_timers[1][_id][T_ENC_BEGIN]) << "	";								//Encode

		//*m_fileDebugOutput << glm::max(m_timers[0][_id][T_NET_BEGIN] - m_timers[0][_id][T_ENC_BEGIN], //TODO: TEMP
		//	0LL) << "	";								//Encode

		*m_fileDebugOutput << glm::max(m_timers[0][_id][T_NET_FIN] - m_timers[0][_id][T_NET_BEGIN] + m_serverDiff,
			m_timers[1][_id][T_NET_FIN] - m_timers[1][_id][T_NET_BEGIN] + m_serverDiff) << "	";				//Network

		*m_fileDebugOutput << glm::max(m_timers[0][_id][T_DEC_FIN] - m_timers[0][_id][T_DEC_BEGIN],
			m_timers[1][_id][T_DEC_FIN] - m_timers[1][_id][T_DEC_BEGIN]) << "	";									//Decode

		//*m_fileDebugOutput << glm::max(m_timers[0][_id][T_DEC_FIN] - m_timers[0][_id][T_DEC_BEGIN], //TODO: TEMP
		//	0LL) << "	";									//Decode

		*m_fileDebugOutput << glm::min(m_timers[0][_id][T_DIS_FIN] - m_timers[0][_id][T_DEC_FIN],
			m_timers[1][_id][T_DIS_FIN] - m_timers[1][_id][T_DEC_FIN]) << "	";									//Display

		*m_fileDebugOutput << m_timers[_eye][_id][T_DIS_FIN] - m_timers[_eye][_id][T_INPUT_BEGIN] << "	";							//Total
		*m_fileDebugOutput << m_serverDiff << "	";																						//Clock diff
		*m_fileDebugOutput << m_ping << "	";																								//Ping


		//long long decoderTime = glm::max(m_timers[0][_id][T_DEC_FIN] - m_timers[0][_id][T_NET_FIN],
		//	m_timers[1][_id][T_DEC_FIN] - m_timers[1][_id][T_NET_FIN]);

		//printf("Decode time %lld\n", decoderTime);

		double fps = 0;

		//for (int i = 0; i < TIMER_ENTRIES; i++)
		//{
		//	fps += m_doubles[_eye][i][0];
		//}

		fps += m_doubles[_eye][_id][T_FPSx90];
		fps += m_doubles[_eye][(_id - 1 + TIMER_ENTRIES) % TIMER_ENTRIES][T_FPSx90];
		fps += m_doubles[_eye][(_id + 1) % TIMER_ENTRIES][T_FPSx90];
		fps /= 3;

		//fps = m_doubles[_eye][_id][0];
		//*m_fileDebugOutput << (fps / TIMER_ENTRIES) << "	";		 																		//FPSx90
		*m_fileDebugOutput << (fps) << "	";		 																						//FPSx90
		*m_fileDebugOutput << m_doubles[_eye][_id][T_RES_FACTOR] << "	";		 																//Resolution factor
		*m_fileDebugOutput << glm::max(0LL, m_timers[_eye][_id][T_RENDER_BEGIN] - m_timers[_eye][_id][T_INPUT_BEGIN] - m_serverDiff) << "	";						//Input delay

		*m_fileDebugOutput << ((m_timers[_eye][_id][T_BANDWIDTH]) * 8.0) / (fps / 90.0) / 1000 << "	";									//Bandwidth kbit per sec
		*m_fileDebugOutput << (m_timers[_eye][_id][T_BANDWIDTH]) << "	";																	//Bandwidth bytes
		*m_fileDebugOutput << (m_timers[_eye][_id][T_PKT_SIZE]) << "	";																	//Packet size
		*m_fileDebugOutput << (m_timers[_eye][_id][T_BIT_RATE]) << "	";																	//Bitrate

		*m_fileDebugOutput << m_timers[_eye][_id][T_INPUT_RESEND] << "	";																	//input nacks
		*m_fileDebugOutput << m_timers[_eye][_id][T_INPUT_REDUNDANT] << "	";															//input redundant

		*m_fileDebugOutput << (m_timers[0][_id][T_PACKETS] + m_timers[1][_id][T_PACKETS]) << "	";															//input redundant
		*m_fileDebugOutput << (m_doubles[_eye][_id][T_MISC]) << "\n";															//input redundant


		if (m_timers[0][_id][T_DIS_FIN] - m_timers[0][_id][T_DEC_FIN] == 0 || 
			m_timers[1][_id][T_DIS_FIN] - m_timers[1][_id][T_DEC_FIN] == 0 ||
			m_timers[0][_id][T_DIS_FIN] == -1 || 
			m_timers[0][_id][T_DEC_FIN] == -1 ||
			m_timers[1][_id][T_DIS_FIN] == -1 ||
			m_timers[1][_id][T_DEC_FIN] == -1 
			)
		{
			printf("Invalid print %lld. This happens if an extra delay gets caught up with.\n", m_timers[_eye][_id][T_TIME_STAMP]);
		}


		//if (m_doubles[_eye][_id][T_RES_FACTOR] != m_previousResLevel)
		//{
		//	m_previousResLevel = m_doubles[_eye][_id][T_RES_FACTOR];
		//	m_framesPrintedSinceResSwitch = 0;
		//	FlushPrinter();
		//}

		//m_framesPrintedSinceResSwitch++;
	}

	int lastId = (actualIndex - 1 + TIMER_ENTRIES) % TIMER_ENTRIES;

	for (int i = 0; i < DEBUG_TIMERS; i++)
	{
		m_timers[1][lastId][i] = -1;
		m_timers[0][lastId][i] = -1;
	}

	prevFrame[_eye] = actualIndex;
}

int Timers::DebugTest(int _eye)
{
	int id = (m_lastPrintedIndex - 1) % TIMER_ENTRIES;

	long long time = m_timers[_eye][id][T_DEC_BEGIN] - m_timers[_eye][id][T_NET_FIN];

	

	if (time > 10000)
	{
		return true;
	}
	return false;
}

// int Timers::NumberOfFramesPrintedThisLevel()
// {
// 	return m_framesPrintedSinceResSwitch;
// }

void Timers::FlushPrinter()
{
	m_fileDebugOutput->flush();
}

//Toss some garbage frames at start
int Timers::HasStartedPrinting()
{
	return m_lastPrintedIndex > (500);
}

long long Timers::PullLongLong(int _eye, int _index, int _id)
{
	return m_timers[_eye][_index % TIMER_ENTRIES][_id];
}

//int Timers::FrameTimeLimit()
//{
//	return m_deltaTime * 11000;
//}