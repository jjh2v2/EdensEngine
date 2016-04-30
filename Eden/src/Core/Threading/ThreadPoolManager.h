#pragma once
#include "Core/Threading/ThreadPool.h"

class ThreadPoolManager
{
public:
	ThreadPoolManager();
	~ThreadPoolManager();

	void Initialize(uint32 numWorkerThreads);

	static ThreadPoolManager *GetSingleton();
	static void DestroySingleton();

	ThreadPool *GetThreadPool()
	{
		return mThreadPool;
	}

private:
	static ThreadPoolManager *mThreadPoolManager;

	ThreadPool *mThreadPool;
};