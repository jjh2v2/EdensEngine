#pragma once
#include <assert.h>
#include "Core/Platform/PlatformCore.h"

template <class T>
class DynamicArray
{
public:

	DynamicArray()
	{
		mMaxSize = 4;
		mArray = new T[mMaxSize];
		mSize = 0;
	}

	DynamicArray(int size)
	{
		mMaxSize = size;
		mArray = new T[size];
		mSize = 0;
	}

	~DynamicArray()
	{
		delete [] mArray;
	}

	T& operator[] (const uint32 index)
	{
		assert(index < mSize);
		return mArray[index];
	}

	void Clear()
	{
		delete [] mArray;
		mMaxSize = 4;
		mArray = new T[mMaxSize];
		mSize = 0;
	}

	void ClearFast()
	{
		mSize = 0;
	}

	void Add(T data)
	{
		mArray[mSize] = data;
		mSize++;

		if(mSize == mMaxSize)
		{
			Resize((int)((float)mMaxSize * 2.0f));
		}
	}

	void Remove(uint32 index)
	{
		//assert(index < mSize);

		memcpy(mArray + index, mArray + index + 1, sizeof(T) * (mSize - index - 1));

		mSize -= 1;
	}

	T RemoveLast()
	{
		mSize--;
		return mArray[mSize];
	}

	T RemoveFast(uint32 index)
	{
		T removed = mArray[index];
		mArray[index] = mArray[mSize - 1];
		mSize--;

		return removed;
	}

	uint32 CurrentSize()
	{
		return mSize;
	}

	uint32 MaxSize()
	{
		return mMaxSize;
	}

	uint32 Resize(const uint32 newSize)
	{
		if(newSize <= mMaxSize){return mMaxSize;}

		T* newArray = new T[newSize];
		for(uint32 i = 0; i < mMaxSize; i++)
		{
			newArray[i] = mArray[i];
		}

		delete [] mArray;
		mArray = newArray;
		mMaxSize = newSize;
		return mMaxSize;
	}

	T* GetInnerArray()
	{
		return mArray;
	}

	T* GetInnerArrayCopy()
	{
		T* copyArray = new T[mSize];
		for(uint32 i = 0; i < mSize; i++)
		{
			copyArray[i] = mArray[i];
		}
		return copyArray;
	}

private:

	T* mArray;
	uint32 mSize;
	uint32 mMaxSize;
};