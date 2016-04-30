#pragma once
#include "Util/Math/MathHelper.h"

template<class T>
class AnimatedValueKey
{
public:
	AnimatedValueKey()
	{
		mTime = 0;
		mValue = T();
	}

	AnimatedValueKey(float time, T value)
	{
		mTime = time;
		mValue = value;
	}

	float GetTime() { return mTime; }
	T GetValue() { return mValue; }

	void SetTime(float time) { mTime = time; }
	void SetValue(T value) { mValue = value; }

	bool operator > (const AnimatedValueKey& right)
	{
		return mTime > right.mTime;
	}

	bool operator < (const AnimatedValueKey& right)
	{
		return mTime < right.mTime;
	}

private:
	float mTime;
	T mValue;
};