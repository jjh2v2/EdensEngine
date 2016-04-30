#pragma once
#include <d3d11.h>
#include <d3dx10math.h>
#include "Util/Math/MathHelper.h"

class Vector4
{
public:
	Vector4()
	{
		X = Y = Z = W = 0;
	}
	Vector4(float x, float y, float z, float w)
	{
		X = x;
		Y = y;
		Z = z;
		W = w;
	}
	static Vector4 Zero()
	{
		return Vector4();
	}
	static Vector4 One()
	{
		return Vector4(1, 1, 1, 1);
	}

	static Vector4 Transform(const Vector4 &point, const D3DXMATRIX &matrix)
	{
		Vector4 result;

		result.X = point.X * matrix._11 + point.Y * matrix._21 + point.Z * matrix._31 + point.W * matrix._41;
		result.Y = point.X * matrix._12 + point.Y * matrix._22 + point.Z * matrix._32 + point.W * matrix._42;
		result.Z = point.X * matrix._13 + point.Y * matrix._23 + point.Z * matrix._33 + point.W * matrix._43;
		result.W = point.X * matrix._14 + point.Y * matrix._24 + point.Z * matrix._34 + point.W * matrix._44;

		return result;
	}

	static Vector4 Scale(const Vector4 &point, float scale)
	{
		return Vector4(point.X * scale, point.Y * scale, point.Z * scale, point.W * scale);
	}

	static Vector4 Rounded(const Vector4 &v)
	{
		return Vector4(MathHelper::Round(v.X), MathHelper::Round(v.Y), MathHelper::Round(v.Z), MathHelper::Round(v.W));
	}

	Vector4 operator+(const Vector4 &rhs)
	{
		Vector4 result(X + rhs.X, Y + rhs.Y, Z + rhs.Z, W + rhs.W);
		return result;
	}

	Vector4 operator+=(const Vector4 &rhs)
	{
		X += rhs.X;
		Y += rhs.Y;
		Z += rhs.Z;
		W += rhs.W;
		return *this;
	}

	Vector4 operator-(const Vector4 &rhs)
	{
		Vector4 result(X - rhs.X, Y - rhs.Y, Z - rhs.Z, W - rhs.W);
		return result;
	}

	Vector4 operator*(const float &rhs)
	{
		Vector4 result(X * rhs, Y * rhs, Z * rhs, W * rhs);
		return result;
	}

	float X;
	float Y;
	float Z;
	float W;
private:
};