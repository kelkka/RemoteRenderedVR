#pragma once
#include <mutex>
#include <assert.h>
#include <fstream>
#include <map>

class StatCalc
{
public:

	//static StatCalc& Get();
	StatCalc(const char * _name);
	~StatCalc();

	void Initialize(long long _val);

	void AddSample(long long _time, long long _rtt, bool _printMe=false, long long _externalTimeConsumed=0);
	bool IsSignificantWaitGamma(long long _time);

	double GetAvg();

	void CalcStdDev(double &stdDev, double sampleMean);

	void AddDebugValue(long long _val);

	void SetUseTimeLimit(bool _use);

	static void SetParameter(const char* _str, double _param);
	static void SetParameterStr(const char * _str, const char* _param);
	static double GetParameter(const char* _str);


	static const char* GetParameterStr(const char * _str);
private:
	

	static const int SYNC_LIST_SIZE = 128;
	long long m_syncWaitList[SYNC_LIST_SIZE];
	int m_syncIndex = 0;
	double m_syncPopMean = 0;
	std::mutex* m_syncMutex;

	unsigned long long m_populationTotal = 0;
	unsigned long long m_populationSize = 0;

	std::ofstream* m_outputFile;
	int m_hits = 0;

	double m_forgettingAvg = 0;

	static std::map<std::string, double> m_parameters;
	static std::map<std::string, std::string> m_parametersStr;

	double m_stdDev = 0;
	double m_shape = 0;
	double m_scale = 0;

	bool m_useTimeLimit =true;

	std::string m_name = "";
};

