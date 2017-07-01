#pragma once
#include "Core/Threading/ThreadPool.h"

class ThreadPoolManager
{
public:
	ThreadPoolManager();
	~ThreadPoolManager();

	void Initialize(uint32 numGeneralPurposeThreads, uint32 numBackgroundWorkerThreads);

	static ThreadPoolManager *GetSingleton();
	static void DestroySingleton();

	ThreadPool *GetGeneralThreadPool()
	{
		return mGeneralThreadPool;
	}

    ThreadPool *GetBackgroundThreadPool()
    {
        return mBackgroundThreadPool;
    }

private:
	static ThreadPoolManager *mThreadPoolManager;

	ThreadPool *mGeneralThreadPool;
    ThreadPool *mBackgroundThreadPool;
};