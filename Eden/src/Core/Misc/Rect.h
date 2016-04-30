#pragma once
#include "Core/Vector/Vector2.h"

class Rect
{
public:
	Rect()
	{
		TopLeft = Vector2(0,0);
		BottomRight = Vector2(0,0);
	}
	Rect(float x1, float y1, float x2, float y2)
	{
		TopLeft = Vector2(x1, y1);
		BottomRight = Vector2(x2, y2);
	}

	Vector2 TopLeft;
	Vector2 BottomRight;
};