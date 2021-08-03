#include "Timers.h"
#include <glm.hpp>
#include <algorithm>

Timers& Timers::Get()
{
	static Timers timers = Timers();
	return timers;
}

Timers::Timers()
{
}

Timers::~Timers()
{
}

long long Timers::Time()
{
	auto timept = std::chrono::high_resolution_clock::now();
	auto now_us = std::chrono::time_point_cast<std::chrono::microseconds>(timept);

	return now_us.time_since_epoch().count();
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

bool Timers::DeadlockCheck(long long _t0, const char* _str)
{
	if (Time() - _t0 > 10000000)
	{
		printf("Deadlock! %s\n", _str);
		return true;
	}
	return false;
}
