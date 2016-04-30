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
		WorkerThread(ThreadPool &s);

		void operator()();
	private:
		ThreadPool &pool;
	};


	ThreadPool(size_t threads);
	~ThreadPool();

	void WaitForAll();

	void AddSingleJob(Job *job);
	void AddJobBatch(JobBatch *jobBatch);

	std::vector<std::thread> workers;
	std::deque<Job *> jobs;

	std::mutex queue_mutex;
	std::mutex batch_mutex;
	std::condition_variable condition;
	std::atomic<bool> stop;
};
