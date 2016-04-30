#pragma once
#include <d3d11.h>
#include <d3dx10math.h>
#include "Core/Vector/Vector4.h"
#include "Util/Math/MathHelper.h"

class Vector3
{
public:
	Vector3()
	{
		X = Y = Z = 0;
	}
	Vector3(float x, float y, float z)
	{
		X = x;
		Y = y;
		Z = z;
	}

	static Vector3 Zero()
	{
		return Vector3();
	}
	static Vector3 One()
	{
		return Vector3(1, 1, 1);
	}

	static Vector3 Cross(const Vector3 &a, const Vector3 &b)
	{
		Vector3 result;

		result.X = a.Y * b.Z - a.Z * b.Y;
		result.Y = a.Z * b.X - a.X * b.Z;
		result.Z = a.X * b.Y - a.Y * b.X;

		return result;
	}

	static Vector3 RandRange(const Vector3 &min, const Vector3 &max)
	{
		return Vector3(MathHelper::RandRange(min.X, max.X),
			MathHelper::RandRange(min.Y, max.Y),
			MathHelper::RandRange(min.Z, max.Z));
	}

	D3DXVECTOR3 AsD3DVector3()
	{
		return D3DXVECTOR3(X, Y, Z);
	}

	Vector3 Normalized()
	{
		float length = (float)sqrt((X * X) + (Y * Y) + (Z * Z));
		return length == 0 ? Vector3::Zero() : Vector3(X/length, Y/length, Z/length);
	}

	static float Length(const Vector3 &v)
	{
		return (float)sqrt((v.X * v.X) + (v.Y * v.Y) + (v.Z * v.Z));
	}

	float Length()
	{
		return (float)sqrt((X * X) + (Y * Y) + (Z * Z));
	}

	Vector3 operator+(const Vector3 &rhs)
	{
		Vector3 result(X + rhs.X, Y + rhs.Y, Z + rhs.Z);
		return result;
	}

	Vector3& operator+=(const Vector3 &rhs)
	{
		X += rhs.X;
		Y += rhs.Y;
		Z += rhs.Z;
		return *this;
	}

	Vector3 operator-(const Vector3 &rhs)
	{
		Vector3 result(X - rhs.X, Y - rhs.Y, Z - rhs.Z);
		return result;
	}

	Vector3 operator*(const float &rhs)
	{
		Vector3 result(X * rhs, Y * rhs, Z * rhs);
		return result;
	}

	Vector3 operator*(const Vector3 &rhs)
	{
		Vector3 result(X * rhs.X, Y * rhs.Y, Z * rhs.Z);
		return result;
	}

	static Vector3 Transform(const Vector3 &point, const D3DXMATRIX &matrix)
	{
		Vector3 result;
		Vector4 temp(point.X, point.Y, point.Z, 1); //need a 4-part vector in order to multiply by a 4x4 matrix
		Vector4 temp2;

		temp2.X = temp.X * matrix._11 + temp.Y * matrix._21 + temp.Z * matrix._31 + temp.W * matrix._41;
		temp2.Y = temp.X * matrix._12 + temp.Y * matrix._22 + temp.Z * matrix._32 + temp.W * matrix._42;
		temp2.Z = temp.X * matrix._13 + temp.Y * matrix._23 + temp.Z * matrix._33 + temp.W * matrix._43;
		temp2.W = temp.X * matrix._14 + temp.Y * matrix._24 + temp.Z * matrix._34 + temp.W * matrix._44;

		result.X = temp2.X/temp2.W; //view projection matrices make use of the W component
		result.Y = temp2.Y/temp2.W;
		result.Z = temp2.Z/temp2.W;

		return result;
	}

	static Vector3 TransformTransposed(const Vector3 &point, const D3DXMATRIX &matrix)
	{
		Vector3 result;
		Vector4 temp(point.X, point.Y, point.Z, 1); //need a 4-part vector in order to multiply by a 4x4 matrix
		Vector4 temp2;

		temp2.X = temp.X * matrix._11 + temp.Y * matrix._12 + temp.Z * matrix._13 + temp.W * matrix._14;
		temp2.Y = temp.X * matrix._21 + temp.Y * matrix._22 + temp.Z * matrix._23 + temp.W * matrix._24;
		temp2.Z = temp.X * matrix._31 + temp.Y * matrix._32 + temp.Z * matrix._33 + temp.W * matrix._34;
		temp2.W = temp.X * matrix._41 + temp.Y * matrix._42 + temp.Z * matrix._43 + temp.W * matrix._44;

		result.X = temp2.X/temp2.W;	//view projection matrices make use of the W component
		result.Y = temp2.Y/temp2.W;
		result.Z = temp2.Z/temp2.W;

		return result;
	}

	static Vector3 FromD3DVector(const D3DXVECTOR3 &v)
	{
		return Vector3(v.x, v.y, v.z);
	}

	static Vector3 GetMiddle(const Vector3 &a, const Vector3 &b)
	{
		return Vector3((a.X + b.X)/2.0f, (a.Y + b.Y)/2.0f, (a.Z + b.Z)/2.0f);
	}

	static float GetDistance(const Vector3 &a, const Vector3 &b)
	{
		return (float)sqrt((a.X - b.X)*(a.X - b.X) + (a.Y - b.Y)*(a.Y - b.Y) + (a.Z - b.Z)*(a.Z - b.Z));
	}

	static Vector3 Lerp(const Vector3 &a, const Vector3 &b, float t)
	{
		return Vector3(MathHelper::Lerp(a.X, b.X, t), MathHelper::Lerp(a.Y, b.Y, t), MathHelper::Lerp(a.Z, b.Z, t));
	}

	static float Dot(const Vector3 &a, const Vector3 &b)
	{
		return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
	}

	float X;
	float Y;
	float Z;
private:
};