#pragma once
#include "Core/Vector/Vector4.h"
#include "Core/Vector/Vector3.h"

class Color
{
public:
	Color()
	{
		R = G = B = A = 1.0f;
	}
	Color(float r, float g, float b, float a)
	{
		R = r;
		G = g;
		B = b;
		A = a;
	}

	Color(Vector4 &v)
	{
		R = v.X;
		G = v.Y;
		B = v.Z;
		A = v.W;
	}

	D3DXVECTOR4 AsD3DVector4()
	{
		return D3DXVECTOR4(R, G, B, A);
	}

	Vector4 AsVector4()
	{
		return Vector4(R, G, B, A);
	}

	Vector3 AsVector3()
	{
		return Vector3(R, G, B);
	}

	D3DXVECTOR3 AsD3DVector3()
	{
		return D3DXVECTOR3(R, G, B);
	}

	static Color Lerp(const Color &a, const Color &b, float t)
	{
		return Color(MathHelper::Lerp(a.R, b.R, t), MathHelper::Lerp(a.G, b.G, t), MathHelper::Lerp(a.B, b.B, t), MathHelper::Lerp(a.A, b.A, t));
	}

	~Color(){}

	Color operator+(const Color &rhs)
	{
		Color result(R + rhs.R, G + rhs.G, B+ rhs.B, A + rhs.A);
		return result;
	}

	bool operator==(const Color &rhs)
	{
		return MathHelper::FloatsAreEqual(R, rhs.R) && MathHelper::FloatsAreEqual(G, rhs.G)
			&& MathHelper::FloatsAreEqual(B, rhs.B) && MathHelper::FloatsAreEqual(A, rhs.A);
	}

	bool operator!=(const Color &rhs)
	{
		return !MathHelper::FloatsAreEqual(R, rhs.R) || !MathHelper::FloatsAreEqual(G, rhs.G)
			|| !MathHelper::FloatsAreEqual(B, rhs.B) || !MathHelper::FloatsAreEqual(A, rhs.A);
	}

	float R;
	float G;
	float B;
	float A;
private:
};
