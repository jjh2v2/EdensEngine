#pragma once
#include <d3d11.h>
#include <d3dx10math.h>
#include <random>
#include "Core/Platform/PlatformCore.h"

static thread_local std::mt19937 gRandomGeneratorEngine;

class MathHelper
{
public:

	MathHelper()
	{
		
	}

	static uint32 AlignU32(uint32 valueToAlign, uint32 alignment)
	{
		alignment -= 1;
		return (uint32)((valueToAlign + alignment) & ~alignment);
	}

    static uint64 AlignU64(uint64 valueToAlign, uint64 alignment)
    {
        alignment -= 1;
        return (uint64)((valueToAlign + alignment) & ~alignment);
    }

	template <class T>
	static T Clamp(T val, T min, T max)
	{
		if (val < min)
		{
			val = min;
		}
		if (val > max)
		{
			val = max;
		}
		return val;
	}

	static float Radian()
	{
		return 0.0174532925f;
	}

	static float Degrees()
	{
		return 57.2957795f;
	}

	template <class T>
	static T Max(T a, T b)
	{
		return a > b ? a : b;
	}

	template <class T>
	static T Min(T a, T b)
	{
		return a < b ? a : b;
	}

	static float Ceil(float val)
	{
		int32 x = (int32) val;
		return ((float)x < val) ? x + 1.0f : x;
	}

	static float Round(float val)
	{
		int32 rounded = (int32)(val - 0.5f);
		return (float)rounded;
	}

	static float Lerp(float a, float b, float t)
	{
		return (1.0f - t) * a + t * b;
	}

	static float Pow(float x, int32 p)
	{
		return (float)pow(x, p);
	}

	static float Pow(float x, float p)
	{
		return (float)pow(x, p);
	}

	static int32 TruncateFloatToInt(float f)
	{
		return (int)f;
	}

	static float GetFraction(float f)
	{
		return f - (float)TruncateFloatToInt(f);
	}

    static uint32 DivideByMultipleOf(uint32 x, uint32 y)
    {
        return (x + y - 1) / y;
    }

	static float FloatsAreEqual(float a, float b, float epsilon = FLT_EPSILON)
	{
		return fabs(a - b) < epsilon;
	}

	static int32 RandRange(int32 min, int32 max)
	{
		std::uniform_int_distribution<int> distribution(min, max);
		return distribution(gRandomGeneratorEngine);
	}

	static float RandRange(float min, float max)
	{
		std::uniform_real_distribution<float> distribution(min, max);
		return distribution(gRandomGeneratorEngine);
	}
};


