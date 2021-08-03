#pragma once
#include <chrono>
#include <fstream>
#include <vector>

class Timers
{
public:
	static Timers& Get();

	~Timers();

	long long Time();

	void SetPing(long long _ping);
	void SetServerTimeDiff(long long _diff);

	long long Ping() { return m_ping; }
	long long ServerDiff() { return m_serverDiff; }

	bool DeadlockCheck(long long _t0, const char* _str);

private:

	Timers();

	static const int TIMER_ENTRIES = 8;
	
	long long m_ping = 0;
	long long m_serverDiff = 0;
	float m_deltaTime = 1;

	int m_diffIndex = 0;
	std::vector<long long> m_diffs;
};


