#include "Core/Threading/ThreadPool.h"

ThreadPool::WorkerThread::WorkerThread(ThreadPool &s)
	: pool(s) 
{
}

void ThreadPool::WorkerThread::operator()()
{
	Job *job = NULL;

	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(pool.queue_mutex);

			// look for a work item
			while (!pool.stop && pool.jobs.empty())
			{
				pool.condition.wait(lock);
			}

			if (pool.stop && pool.jobs.empty()) // exit if the pool is stopped and we have no more jobs
			{
				return;
			}

			job = pool.jobs.front();
			pool.jobs.pop_front();
		}

		job->Execute();

		if (job->GetJobBatch() != NULL)
		{
			std::unique_lock<std::mutex> lock(pool.batch_mutex);

			JobBatch *jobBatch = job->GetJobBatch();
			jobBatch->CompletedJob(job);

			if (jobBatch->GetNumJobs() == 0)
			{
				jobBatch->SetCompletedAllJobs();
			}
		}
	}
}

ThreadPool::ThreadPool(size_t threads)
	: stop(false)
{
	for (size_t i = 0; i < threads; ++i)
	{
		workers.push_back(std::thread(WorkerThread(*this)));
	}
}

ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}

	condition.notify_all();

	for (size_t i = 0; i < workers.size(); ++i)
	{
		workers[i].join();
	}
}

void ThreadPool::WaitForAll()
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}

	condition.notify_all();

	for (size_t i = 0; i < workers.size(); ++i)
	{
		workers[i].join();
	}
}

void ThreadPool::AddSingleJob(Job *job)
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		jobs.push_back(job);
	}

	condition.notify_one();
}

void ThreadPool::AddJobBatch(JobBatch *jobBatch)
{
	if (jobBatch->GetNumJobs() == 0)	//no jobs, don't bother
	{
		jobBatch->SetCompletedAllJobs();
	}

	{
		std::unique_lock<std::mutex> batchLock(batch_mutex);

		uint32 jobCount = jobBatch->GetNumJobs();
		for (uint32 i = 0; i < jobCount; i++)
		{
			Job *job = jobBatch->GetBatchJob(i);
			AddSingleJob(job);
		}
	}
}