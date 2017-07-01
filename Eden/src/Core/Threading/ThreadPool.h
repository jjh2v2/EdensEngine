#pragma once
#include <windows.h>
#include <thread>
#include <mutex>
#include <deque>
#include <atomic>
#include <vector>
#include "Core/Threading/Job.h"

class ThreadPool
{
public:
	class WorkerThread
	{
	public:
		WorkerThread(ThreadPool &threadPool);
		void operator()();

	private:
		ThreadPool &mThreadPool;
	};


	ThreadPool(uint32 numThreads);
	~ThreadPool();

	void WaitForAll();
	void AddSingleJob(Job *job);
	void AddJobBatch(JobBatch *jobBatch);

	std::vector<std::thread> mWorkerThreads;
	std::deque<Job *> mJobs;

	std::mutex mJobQueueMutex;
	std::mutex mJobBatchMutex;
	std::condition_variable mCondition;
	std::atomic<bool> mStop;
};
