#pragma once
#include <random>
#include "Core/Vector4.h"
#include "Core/Vector3.h"
#include "Core/Vector2.h"

class RandomGen
{
public:
	RandomGen()
	{
		
	}

	~RandomGen()
	{
		
	}

	float RandRange(float min, float max)
	{
		std::uniform_real_distribution<float> distribution(min, max);
		return distribution(mRandomGeneratorEngine);
	}

	int RandRange(int min, int max)
	{
		std::uniform_int_distribution<int> distribution(min, max);
		return distribution(mRandomGeneratorEngine);
	}

	Vector4 RandRange(Vector4 min, Vector4 max)
	{
		return Vector4(RandRange(min.X, max.X), RandRange(min.Y, max.Y), RandRange(min.Z, max.Z), RandRange(min.W, max.W));
	}

	Vector3 RandRange(Vector3 min, Vector3 max)
	{
		return Vector3(RandRange(min.X, max.X), RandRange(min.Y, max.Y), RandRange(min.Z, max.Z));
	}

	Vector2 RandRange(Vector2 min, Vector2 max)
	{
		return Vector2(RandRange(min.X, max.X), RandRange(min.Y, max.Y));
	}

private:
	std::mt19937 mRandomGeneratorEngine;
};