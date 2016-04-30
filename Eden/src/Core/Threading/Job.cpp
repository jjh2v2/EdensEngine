#include "Core/Threading/Job.h"

Job::Job(JobBatch *batch /* = NULL */)
{
	mJobBatch = batch;
}

JobBatch::JobBatch()
{
	mJobsCompleted = false;
}

void JobBatch::AddBatchJob(Job *job)
{
	job->SetJobBatch(this);
	mJobs.Add(job);
}

void JobBatch::CompletedJob(Job *job)
{
	for (uint32 i = 0; i < mJobs.CurrentSize(); i++)
	{
		if (mJobs[i] == job)
		{
			mJobs.RemoveFast(i);
			return;
		}
	}

	bool didNotFind = false;
	assert(didNotFind);
}

void JobBatch::SetCompletedAllJobs()
{
	mJobsCompleted = true;
}

uint32 JobBatch::GetNumJobs()
{
	return mJobs.CurrentSize();
}

Job *JobBatch::GetBatchJob(uint32 index)
{
	return mJobs[index];
}

void JobBatch::WaitForBatch()
{
	while (!mJobsCompleted)
	{

	}
}