#include "Core/Threading/ThreadPoolManager.h"

ThreadPoolManager *ThreadPoolManager::mThreadPoolManager = NULL;

ThreadPoolManager::ThreadPoolManager()
{
	mThreadPoolManager = NULL;
}

ThreadPoolManager::~ThreadPoolManager()
{
	delete mGeneralThreadPool;
    mGeneralThreadPool = NULL;

    delete mBackgroundThreadPool;
    mBackgroundThreadPool = NULL;

	mThreadPoolManager = NULL;
}

void ThreadPoolManager::Initialize(uint32 numGeneralPurposeThreads, uint32 numBackgroundWorkerThreads)
{
	mGeneralThreadPool = new ThreadPool(numGeneralPurposeThreads);
    mBackgroundThreadPool = new ThreadPool(numBackgroundWorkerThreads);
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