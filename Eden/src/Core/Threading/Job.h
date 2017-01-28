#pragma once
#include "Core/Containers/DynamicArray.h"
#include "Core/Platform/PlatformCore.h"

class JobBatch;

class Job
{
public:
	Job(JobBatch *batch = NULL);
	virtual void Execute() = 0;
	void SetJobBatch(JobBatch *batch) { mJobBatch = batch; }
	JobBatch *GetJobBatch() { return mJobBatch; }

private:
	JobBatch *mJobBatch;
};

class JobBatch
{
public:
	JobBatch();

	void AddBatchJob(Job *job);
	void CompletedJob(Job *job);
	void SetCompletedAllJobs();

	uint32 GetNumJobs();
	Job *GetBatchJob(uint32 index);

	void WaitForBatch();

private:
	DynamicArray<Job *> mJobs;
	volatile bool mJobsCompleted; //TDA: Replace this. Volatile isn't the right answer for this
};