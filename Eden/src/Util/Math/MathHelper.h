#pragma once
#include <d3d11.h>
#include <d3dx10math.h>
#include <random>

static thread_local std::mt19937 gRandomGeneratorEngine;

class MathHelper
{
public:

	MathHelper()
	{
		
	}

	static float Clamp(float val, float min, float max)
	{
		if(val < min)
		{
			val = min;
		}
		if(val > max)
		{
			val = max;
		}
		return val;
	}

	static int Clamp(int val, int min, int max)
	{
		if(val < min)
		{
			val = min;
		}
		if(val > max)
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

	static D3DXVECTOR3 GetNormalized(D3DXVECTOR3 vector3)
	{
		float length = (float)sqrt((vector3.x * vector3.x) + (vector3.y * vector3.y) + (vector3.z * vector3.z));
		return length == 0 ? D3DXVECTOR3(0,0,0) : D3DXVECTOR3(vector3.x/length, vector3.y/length, vector3.y/length);
	}

	static size_t MaxSize(size_t a, size_t b)
	{
		return a > b ? a : b;
	}

	static size_t MinSize(size_t a, size_t b)
	{
		return a < b ? a : b;
	}

	static float Max(float a, float b)
	{
		return a > b ? a : b;
	}

	static float Min(float a, float b)
	{
		return a < b ? a : b;
	}

	static int Max(int a, int b)
	{
		return a > b ? a : b;
	}

	static int Min(int a, int b)
	{
		return a < b ? a : b;
	}

	static float Ceil(float val)
	{
		int x = (int) val;
		return ((float)x < val) ? x + 1.0f : x;
	}

	static float Round(float val)
	{
		int rounded = (int)(val - 0.5f);
		return (float)rounded;
	}

	static float Lerp(float a, float b, float t)
	{
		return (1.0f - t) * a + t * b;
	}

	static float Pow(float x, int p)
	{
		return (float)pow(x, p);
	}

	static float Pow(float x, float p)
	{
		return (float)pow(x, p);
	}

	static int TruncateFloatToInt(float f)
	{
		return (int)f;
	}

	static float GetFraction(float f)
	{
		return f - (float)TruncateFloatToInt(f);
	}

	static float FloatsAreEqual(float a, float b, float epsilon = FLT_EPSILON)
	{
		return fabs(a - b) < epsilon;
	}

	static int RandRange(int min, int max)
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


