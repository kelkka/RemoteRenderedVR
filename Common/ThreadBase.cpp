#include "ThreadBase.h"
#include <omp.h>

void ThreadBase::DestroyWorker()
{
	printf("Worker destroy %p\n",this);
	m_quit = true;
	GoProcess();
	if(m_worker.joinable())
		m_worker.join();
}

void ThreadBase::RunProcess()
{
	printf("Worker start %p\n", this);

	double t_0 = 0;

	while (m_quit == false)
	{
		std::unique_lock<std::mutex> waitLock(m_runMutex);
		m_condition.wait(waitLock, [this]{ return m_go; });
		m_go = false;
		//std::lock_guard<std::mutex> guard(m_runMutex);

		if (m_quit == true)
			break;

		SpecificProcess();

		waitLock.unlock();
	}

	printf("Worker complete %p\n", this);
}


void ThreadBase::GoProcess()
{
	std::lock_guard<std::mutex> guard(m_runMutex);
	m_go = true;

	m_condition.notify_one();
}