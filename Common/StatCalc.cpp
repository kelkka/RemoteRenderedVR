#include "StatCalc.h"
#include <stats-master/stats.hpp>

#define PRINT_STATISTICS

std::map<std::string, double> StatCalc::m_parameters;
std::map<std::string, std::string> StatCalc::m_parametersStr;

StatCalc::StatCalc(const char* _name)
{
	for (int i = 0; i < SYNC_LIST_SIZE; i++)
	{
		m_syncWaitList[i] = 11111 + (rand()% 100);
	}

	m_forgettingAvg = 11111;

	std::string fileName = "../content/output/Stat_";
	fileName += _name;
	fileName += ".txt";

#ifdef PRINT_STATISTICS
	m_outputFile = new std::ofstream(fileName.c_str(), std::ofstream::out);
	*m_outputFile << "Time	Average	Consumed	Hit	Shape	Scale	RTT";
#endif

	m_syncMutex = new std::mutex();

	//m_alarmParameter = _name;
	//m_alarmParameter += "_alarm";

	//m_forgetParameter = _name;
	//m_forgetParameter += "_forget";

	m_name = _name;
}

StatCalc::~StatCalc()
{
#ifdef PRINT_STATISTICS
	m_outputFile->close();
	delete m_outputFile;
#endif
	delete m_syncMutex;
}

void StatCalc::Initialize(long long _val)
{
	for (int i = 0; i < SYNC_LIST_SIZE; i++)
	{
		m_syncWaitList[i] = _val + (rand() % 100);
	}

	m_forgettingAvg = (double)_val;
}

void StatCalc::AddSample(long long _time, long long _rtt, bool _printMe, long long _externalTimeConsumed)
{
	std::lock_guard<std::mutex> guard(*m_syncMutex);

#ifdef PRINT_STATISTICS
	if (_printMe)
	{
		*m_outputFile << "\n" << _time << "	" << (long long)m_forgettingAvg << "	" << _externalTimeConsumed << "	";

		//if (m_hits > 0)
		{
			*m_outputFile << m_hits;
		}

		*m_outputFile << "	" << m_shape << "	" << m_scale << "	" << _rtt;
	}
	
	m_hits = 0;
#endif

	if (m_useTimeLimit)
	{
		if (_time > 50000)
		{
			_time = 50000;
		}
	}

	m_syncWaitList[m_syncIndex] = _time;
	m_syncIndex = (m_syncIndex + 1) % SYNC_LIST_SIZE;

	m_forgettingAvg = 0;

	for (int i = 0; i < SYNC_LIST_SIZE; i++)
	{
		m_forgettingAvg += m_syncWaitList[i];
	}

	m_forgettingAvg /= SYNC_LIST_SIZE;

	if (m_forgettingAvg <= 0 || _time <= 0)
		return;

	CalcStdDev(m_stdDev, m_forgettingAvg);

}

bool StatCalc::IsSignificantWaitGamma(long long _time)
{
	double stdDev = 0;
	double mean = 0;
	//double popMean = (double)m_populationTotal / (double)m_populationSize;
	{
		std::lock_guard<std::mutex> guard(*m_syncMutex);
		stdDev = m_stdDev;
		mean = m_forgettingAvg;
	}

	if (stdDev == 0)
	{
		//printf("zero stdve \n");
		return false;
	}
	
	m_shape = (mean*mean) / (stdDev*stdDev);
	m_scale = (stdDev*stdDev) / mean;
	
	if (m_scale > 1000)
		m_scale = 1000;

	double p = stats::pgamma((double)_time, m_shape, m_scale);

	if (isinf(p))
	{
		printf("p is inf!\n");
		return false;
	}

	if (_time > 100000)
	{
		return true;
	}

	//printf("p-value gamma %f for %lld | avg: %f | shape %f | scale %f\n", p, _time, mean, shape, scale);

	if ((p > m_parameters["GAMMA_P_VAL"] && _time > mean) || p < 0)
	{
#ifdef PRINT_STATISTICS
		m_hits++;
#endif
		//printf("p-value gamma %f for %lld | avg: %f | shape %f | scale %f\n", p, _time, mean, m_shape, m_scale);
		return true;
	}

	return false;
}

double StatCalc::GetAvg()
{
	return m_forgettingAvg;
}

void StatCalc::CalcStdDev(double &stdDev, double sampleMean)
{
	for (int i = 0; i < SYNC_LIST_SIZE; i++)
	{
		stdDev += pow((double)m_syncWaitList[i] - sampleMean, 2);
	}
	stdDev /= SYNC_LIST_SIZE;
	stdDev = sqrt(stdDev);
}

void StatCalc::AddDebugValue(long long _val)
{
#ifdef PRINT_STATISTICS
	std::lock_guard<std::mutex> guard(*m_syncMutex);
	*m_outputFile << "	" << _val;
#endif
}

void StatCalc::SetUseTimeLimit(bool _use)
{
	m_useTimeLimit = _use;
}

//Sorry, this class also contains config parameters of the whole project. Should be moved to a new config class obviously
//These are read in netbase.cpp too...

void StatCalc::SetParameter(const char * _str, double _param)
{
	m_parameters[_str] = _param;

	printf("%s is now %f\n", _str, _param);
}

void StatCalc::SetParameterStr(const char * _str, const char* _param)
{
	m_parametersStr[_str] = _param;

	printf("%s is now %s\n", _str, _param);
}

double StatCalc::GetParameter(const char * _str)
{
	if (m_parameters.find(_str) == m_parameters.end())
	{
		printf("Invalid parameter! %s\n", _str);
		return -1;
	}

	return m_parameters[_str];
}

const char* StatCalc::GetParameterStr(const char * _str)
{
	if (m_parametersStr.find(_str) == m_parametersStr.end())
	{
		printf("Invalid parameter! %s\n", _str);
		return "";
	}

	return m_parametersStr[_str].c_str();
}
