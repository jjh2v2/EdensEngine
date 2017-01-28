#pragma once
#pragma comment(lib, "winmm.lib")

#include <windows.h>
#include <mmsystem.h>
#include "Core/Platform/PlatformCore.h"

class GameTimer
{
public:
	GameTimer()
	{
		Reset();
	}

	~GameTimer()
	{

	}

	void Reset()
	{
		mFramesPerSecond = 0;
		mFPSCounter = 0;
		mFPSStartTime = timeGetTime();

		QueryPerformanceFrequency((LARGE_INTEGER*)&mTimerFrequency);
		mTimerTicksPerMillisecond = (float)(mTimerFrequency / 1000);
		QueryPerformanceCounter((LARGE_INTEGER*)&mTimerStartTime);
	}

	void Frame()
	{
		mFPSCounter++;

		if(timeGetTime() >= (mFPSStartTime + 1000))
		{
			mFramesPerSecond = mFPSCounter;
			mFPSCounter = 0;
			mFPSStartTime = timeGetTime();
		}

		int64 currentTime;

		QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
		mTimerFrameTime = ((float)(currentTime - mTimerStartTime)) / mTimerTicksPerMillisecond;
		mTimerStartTime = currentTime;
	}

	int32 GetFPS()
	{
		return mFramesPerSecond;
	}

	float GetTime()
	{
		return mTimerFrameTime;
	}

private:
	int32 mFramesPerSecond;
	int32 mFPSCounter;
	unsigned long mFPSStartTime;

	int64 mTimerFrequency;
	float mTimerTicksPerMillisecond;
	int64 mTimerStartTime;
	float mTimerFrameTime;
};