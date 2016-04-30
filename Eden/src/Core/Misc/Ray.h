#pragma once
#include "Core/Vector/Vector3.h"

class Ray
{
public:
	Ray(){};
	Ray(Vector3 origin, Vector3 direction)
	{
		Origin = origin;
		Direction = direction;
	}
	~Ray(){};

	Vector3 Origin;
	Vector3 Direction;
};