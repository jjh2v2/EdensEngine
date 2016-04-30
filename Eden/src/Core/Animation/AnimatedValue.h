#pragma once
#include "Core/Animation/AnimatedValueKey.h"
#include "Core/Containers/DynamicArray.h"
#include "Util/Math/MathHelper.h"
#include "Util/General/Sort.h"
#include "Util/General/Serializer.h"

template <class T>
class AnimatedValue
{
public:
	AnimatedValue()
	{
		mLoop = false;
		mCurrentTime = -1.0f;
	}

	~AnimatedValue()
	{
		mKeys.Clear();
	}

	void SetLoop(bool loop) { mLoop = loop; }

	void AddKey(float time, T value, bool sort = false)
	{
		mKeys.Add(AnimatedValueKey<T>(time, value));

		if (sort)
		{
			QuickSort<AnimatedValueKey<T>>(mKeys.GetInnerArray(), 0, mKeys.CurrentSize() - 1);
		}
	}

	void GetAnimationFramesForTime(float time, uint32 &prevFrame, uint32 &nextFrame)
	{
		uint32 begin = 0;
		uint32 end = mKeys.CurrentSize() - 1;

		while (begin != end)
		{
			int middle = (begin + end) / 2;
			if (mKeys[middle].GetTime() < time)
			{
				begin = middle + 1;
			}
			else
			{
				end = middle;
			}
		}

		prevFrame = begin > 0 ? begin - 1 : 0;
		nextFrame = begin;
	}

	T AnimatedValue::GetValueByDelta(float delta)
	{
		uint32 numKeys = mKeys.CurrentSize();

		if (numKeys < 2)
		{
			return T(); // we need to have at least 2 keys
		}

		float firstFrameTime = mKeys[0].GetTime();
		float lastFrameTime = mKeys[numKeys - 1].GetTime();

		if (mCurrentTime >= lastFrameTime)
		{
			if (mLoop)
			{
				mCurrentTime = firstFrameTime;
			}
			else
			{
				return mKeys[numKeys - 1].GetValue();
			}
		}

		if (mCurrentTime <= firstFrameTime)
		{
			mCurrentTime = firstFrameTime + delta;
			return mKeys[0].GetValue();
		}

		uint32 prevFrame = 0;
		uint32 nextFrame = 0;

		GetAnimationFramesForTime(mCurrentTime, prevFrame, nextFrame);

		float timeBetweenFrames = mKeys[nextFrame].GetTime() - mKeys[prevFrame].GetTime();
		float percentageThroughFrame = timeBetweenFrames <= 0 ? 0 : (mCurrentTime - mKeys[prevFrame].GetTime()) / timeBetweenFrames;

		T keyValue = mKeys[nextFrame].GetValue() * percentageThroughFrame + mKeys[prevFrame].GetValue() * (1.0f - percentageThroughFrame);

		mCurrentTime += delta;

		return keyValue;
	}


	T AnimatedValue::GetValueForTime(float time)
	{
		uint32 numKeys = mKeys.CurrentSize();

		if (numKeys < 2)
		{
			return T(); // we need to have at least 2 keys
		}

		if (time <= mKeys[0].GetTime())
		{
			return mKeys[0].GetValue();
		}

		if (time >= mKeys[numKeys - 1].GetTime())
		{
			return mKeys[numKeys - 1].GetValue();
		}

		uint32 prevFrame = 0;
		uint32 nextFrame = 0;

		GetAnimationFramesForTime(time, prevFrame, nextFrame);

		float timeBetweenFrames = mKeys[nextFrame].GetTime() - mKeys[prevFrame].GetTime();
		float percentageThroughFrame = (time - mKeys[prevFrame].GetTime()) / timeBetweenFrames;

		T keyValue = mKeys[nextFrame].GetValue() * percentageThroughFrame + mKeys[prevFrame].GetValue() * (1.0f - percentageThroughFrame);

		return keyValue;
	}

	void Serialize(const char *file)
	{
		Serializer serializer;
		serializer.BeginSerialization(file);
		serializer.SerializeBool(mLoop);
		serializer.SerializeFloat(mCurrentTime);

		uint32 numKeys = mKeys.CurrentSize();
		serializer.SerializeU32(numKeys);
		
		for (uint32 i = 0; i < mKeys.CurrentSize(); i++)
		{
			float time = mKeys[i].GetTime();
			T keyValue = mKeys[i].GetValue();
			serializer.SerializeFloat(time);
			serializer.SerializeGeneric((void*)&keyValue, sizeof(T));
		}
		serializer.EndSerialization();
	}

	void Serialize(Serializer &serializer)
	{
		serializer.SerializeBool(mLoop);
		serializer.SerializeFloat(mCurrentTime);

		uint32 numKeys = mKeys.CurrentSize();
		serializer.SerializeU32(numKeys);

		for (uint32 i = 0; i < mKeys.CurrentSize(); i++)
		{
			float time = mKeys[i].GetTime();
			T keyValue = mKeys[i].GetValue();
			serializer.SerializeFloat(time);
			serializer.SerializeGeneric((void*)&keyValue, sizeof(T));
		}
	}

	void Deserialize(const char *file)
	{
		Serializer serializer;
		serializer.BeginDeserialization(file);
		serializer.DeserializeBool(mLoop);
		serializer.DeserializeFloat(mCurrentTime);

		uint32 numKeys = 0;
		serializer.DeserializeU32(numKeys);

		for (uint32 i = 0; i < numKeys; i++)
		{
			float time = 0;
			T keyValue;
			serializer.DeserializeFloat(time);
			serializer.DeserializeGeneric((void*)&keyValue, sizeof(T));
			AddKey(time, keyValue);
		}
		serializer.EndDeserialization();
	}

	void Deserialize(Serializer &serializer)
	{
		serializer.DeserializeBool(mLoop);
		serializer.DeserializeFloat(mCurrentTime);

		uint32 numKeys = 0;
		serializer.DeserializeU32(numKeys);

		for (uint32 i = 0; i < numKeys; i++)
		{
			float time = 0;
			T keyValue;
			serializer.DeserializeFloat(time);
			serializer.DeserializeGeneric((void*)&keyValue, sizeof(T));
			AddKey(time, keyValue);
		}
	}

	void CopyTo(AnimatedValue<T> &animatedValue)
	{
		animatedValue.mLoop = mLoop;
		animatedValue.mCurrentTime = mCurrentTime;

		for (uint32 i = 0; i < mKeys.CurrentSize(); i++)
		{
			animatedValue.AddKey(mKeys[i].GetTime(), mKeys[i].GetValue());
		}
	}

	uint32 GetNumKeys(){ return mKeys.CurrentSize(); }

	T GetValueForKey(uint32 keyIndex)
	{
		return mKeys[keyIndex].GetValue();
	}

	void SetValueForKey(uint32 keyIndex, T value)
	{
		mKeys[keyIndex].SetValue(value);
	}

private:
	bool mLoop;
	float mCurrentTime;
	DynamicArray<AnimatedValueKey<T>> mKeys;
};