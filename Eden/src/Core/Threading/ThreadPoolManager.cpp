#include "Core/Threading/ThreadPoolManager.h"

ThreadPoolManager *ThreadPoolManager::mThreadPoolManager = NULL;

ThreadPoolManager::ThreadPoolManager()
{
	mThreadPoolManager = NULL;
}

ThreadPoolManager::~ThreadPoolManager()
{
	delete mThreadPool;
	mThreadPool = NULL;
	mThreadPoolManager = NULL;
}

void ThreadPoolManager::Initialize(uint32 numWorkerThreads)
{
	mThreadPool = new ThreadPool(numWorkerThreads);
}

ThreadPoolManager *ThreadPoolManager::GetSingleton()
{
	if (!mThreadPoolManager)
	{
		mThreadPoolManager = new ThreadPoolManager();
	}

	return mThreadPoolManager;
}

void ThreadPoolManager::DestroySingleton()
{
	if (mThreadPoolManager)
	{
		delete mThreadPoolManager;
		mThreadPoolManager = NULL;
	}
}