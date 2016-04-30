#pragma once
#include <d3d11.h>
#include <d3dx10math.h>
#include "Util/Math/MathHelper.h"

class Vector2
{
public:
	Vector2()
	{
		X = Y = 0;
	}
	Vector2(float x, float y)
	{
		X = x;
		Y = y;
	}
	static Vector2 Zero()
	{
		return Vector2();
	}
	static Vector2 One()
	{
		return Vector2(1, 1);
	}

	D3DXVECTOR2 AsD3DVector2()
	{
		return D3DXVECTOR2(X, Y);
	}

	static Vector2 FromD3DVector(const D3DXVECTOR2 &v)
	{
		return Vector2(v.x, v.y);
	}

	static Vector2 RandRange(const Vector2 &min, const Vector2 &max)
	{
		return Vector2(MathHelper::RandRange(min.X, max.X),
			MathHelper::RandRange(min.Y, max.Y));
	}

	Vector2 operator+(const Vector2 &rhs)
	{
		Vector2 result(X + rhs.X, Y + rhs.Y);
		return result;
	}

	Vector2 operator+=(const Vector2 &rhs)
	{
		X += rhs.X;
		Y += rhs.Y;
		return *this;
	}

	Vector2 operator-(const Vector2 &rhs)
	{
		Vector2 result(X - rhs.X, Y - rhs.Y);
		return result;
	}

	Vector2 operator*(const float &rhs)
	{
		Vector2 result(X * rhs, Y * rhs);
		return result;
	}

	float X;
	float Y;
private:
};