/*
* 
*/

//For statistics printing for experiments

#pragma once
#include <chrono>
#include <fstream>
#include <vector>

enum TIMER_NAMES
{
	T_FPSx90,
	T_RES_FACTOR,
	T_MISC,
	T_PACKETS,
	T_IS_UDP,
	T_TIME_STAMP,
	T_VIDEO_RE_SENDS,
	T_VIDEO_REDUNDANT,
	T_JITTER,
	T_INPUT_BEGIN,
	T_RENDER_BEGIN,
	T_ENC_BEGIN,
	T_NET_BEGIN,
	T_NET_FIN,
	T_DEC_BEGIN,
	T_DEC_FIN,
	T_DIS_FIN,
	T_BANDWIDTH,
	T_PKT_SIZE,
	T_BIT_RATE,
	T_INPUT_RESEND,
	T_INPUT_REDUNDANT,
	T_MAX,
};

class Timers
{
public:
	static Timers& Get();
	Timers();
	~Timers();

	void Init(int _width, int _height);

	void Destroy();

	void PushFloat(int _eye, int _identifier, int _frameIndex, double _val);
	void AddLongLong(int _eye, int _identifier, int _frameIndex, long long _val);
	void PushLongLong(int _eye, int _identifier, int _frameIndex, long long _val);

	void Print(int _id);
	void DoPrint(int _id, int actualIndex, int _eye);
	int DebugTest(int _eye);

	void FlushPrinter();
	int HasStartedPrinting();

	long long PullLongLong(int _eye, int _index, int _id);

	long long Time();

	void SetPing(long long _ping);
	void SetServerTimeDiff(long long _diff);

	long long GetServerDiff();
	long long Ping() { return m_ping; }
	long long ServerDiff() { return m_serverDiff; }

	bool DeadlockCheck(long long _t0, const char* _str);


	int PrintLag() {return PRINT_LAG; }

private:

	std::ofstream* m_fileDebugOutput;

	static const int TIMER_ENTRIES = 8;
	static const int DEBUG_TIMERS = T_MAX;
	volatile long long m_timers[2][TIMER_ENTRIES][DEBUG_TIMERS];
	double m_doubles[2][TIMER_ENTRIES][4];

	long long m_ping = 0;
	long long m_serverDiff = 0;
	float m_deltaTime = 1;

	int m_diffIndex = 0;
	std::vector<long long> m_diffs;

	int m_lastIndex = 0;
	int m_lastPrintedIndex = 0;

	const int PRINT_LAG = 1;
};


