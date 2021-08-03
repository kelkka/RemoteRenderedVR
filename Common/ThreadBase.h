#pragma once

#include <fstream>
#include <iostream>
#include <cuda.h>

#include <memory>

#include <mutex>
#include <CudaError.h>

class ThreadBase
{
public:
	virtual void RunProcess();
	void GoProcess();

	virtual void SpecificProcess() = 0;
	void DestroyWorker();

protected:
	volatile bool m_quit = false;
	volatile bool m_go = false;
	
	std::mutex m_runMutex;
	std::condition_variable m_condition;

	std::thread m_worker;

private:

};

