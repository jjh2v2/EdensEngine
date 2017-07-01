#include "Core/Threading/ThreadPool.h"

ThreadPool::WorkerThread::WorkerThread(ThreadPool &threadPool)
	: mThreadPool(threadPool)
{
}

void ThreadPool::WorkerThread::operator()()
{
	Job *job = NULL;

	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(mThreadPool.mJobQueueMutex);

			// wait for work if there isn't any right now
			while (!mThreadPool.mStop && mThreadPool.mJobs.empty())
			{
				mThreadPool.mCondition.wait(lock);
			}

			if (mThreadPool.mStop && mThreadPool.mJobs.empty())
			{
				return;
			}

			job = mThreadPool.mJobs.front();
			mThreadPool.mJobs.pop_front();
		}

		job->Execute();

		if (job->GetJobBatch() != NULL)
		{
			std::unique_lock<std::mutex> lock(mThreadPool.mJobBatchMutex);

			JobBatch *jobBatch = job->GetJobBatch();
			jobBatch->CompletedJob(job);

			if (jobBatch->GetNumJobs() == 0)
			{
				jobBatch->SetCompletedAllJobs();
			}
		}
	}
}

ThreadPool::ThreadPool(uint32 numThreads)
	: mStop(false)
{
	for (uint32 i = 0; i < numThreads; i++)
	{
		mWorkerThreads.push_back(std::thread(WorkerThread(*this)));
	}
}

ThreadPool::~ThreadPool()
{
    WaitForAll();
}

void ThreadPool::WaitForAll()
{
    {
        std::unique_lock<std::mutex> lock(mJobQueueMutex);
        mStop = true;
    }
	
	mCondition.notify_all();

	for (size_t i = 0; i < mWorkerThreads.size(); ++i)
	{
		mWorkerThreads[i].join();
	}
}

void ThreadPool::AddSingleJob(Job *job)
{
	{
		std::unique_lock<std::mutex> lock(mJobQueueMutex);
		mJobs.push_back(job);
	}

	mCondition.notify_one();
}

void ThreadPool::AddJobBatch(JobBatch *jobBatch)
{
	if (jobBatch->GetNumJobs() == 0)	//no jobs, don't bother
	{
		jobBatch->SetCompletedAllJobs();
	}

	{
		std::unique_lock<std::mutex> batchLock(mJobBatchMutex);

		uint32 jobCount = jobBatch->GetNumJobs();
		for (uint32 i = 0; i < jobCount; i++)
		{
			Job *job = jobBatch->GetBatchJob(i);
			AddSingleJob(job);
		}
	}
}