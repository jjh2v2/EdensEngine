#pragma once
#include "Core/Vector/Vector3.h"
#include "Core/Misc/Ray.h"

class Box
{
public:
	Box()
	{
		X = Y = Z = 0.0f;
		Width = Height = Depth = 1.0f;
	}

	Box(float x, float y, float z, float width, float height, float depth)
	{
		X = x;
		Y = y;
		Z = z;
		Width = width;
		Height = height;
		Depth = depth;
	}

	Vector3 GetMin()
	{
		return Vector3(X,Y,Z);
	}

	Vector3 GetMax()
	{
		return Vector3(X+Width, Y+Height, Z+Depth);
	}

	Vector3 GetXYZ()
	{
		return Vector3(X, Y, Z);
	}

	void Set(float x, float y, float z, float width, float height, float depth)
	{
		X = x;
		Y = y;
		Z = z;
		Width = width;
		Height = height;
		Depth = depth;
	}

	void SetXYZ(Vector3 point)
	{
		SetXYZ(point.X, point.Y, point.Z);
	}

	void SetXYZ(float x, float y, float z)
	{
		X = x;
		Y = y;
		Z = z;
	}

	Vector3 GetBoundCenter()
	{
		Vector3 center(X + Width/2.0f, Y + Height/2.0f, Z + Depth/2.0f);
		return center;
	}

	Vector3 GetSize()
	{
		return Vector3(Width, Height, Depth);
	}

	bool IsCollidingWith(const Vector3 &point)
	{
		return IsCollidingWith(point.X, point.Y, point.Z);
	}

	bool IsCollidingWith(float x, float y, float z)
	{
		if(x >= X && x <= (X + Width) && y >= Y && y <= (Y + Height) && z >= Z && z <= (Z + Depth))
		{
			return true;
		}
		return false;
	}

	bool IsCollidingWith(const Ray &ray)
	{
		float tmin, tmax, tymin, tymax, tzmin, tzmax;
		if (ray.Direction.X >= 0) 
		{
			tmin = (X - ray.Origin.X) / ray.Direction.X;
			tmax = ((X + Width) - ray.Origin.X) / ray.Direction.X;
		}
		else 
		{
			tmin = ((X + Width) - ray.Origin.X) / ray.Direction.X;
			tmax = (X - ray.Origin.X) / ray.Direction.X;
		}
		if (ray.Direction.Y >= 0) 
		{
			tymin = (Y - ray.Origin.Y) / ray.Direction.Y;
			tymax = ((Y + Height) - ray.Origin.Y) / ray.Direction.Y;
		}
		else 
		{
			tymin = ((Y + Height) - ray.Origin.Y) / ray.Direction.Y;
			tymax = (Y - ray.Origin.Y) / ray.Direction.Y;
		}
		if ( (tmin > tymax) || (tymin > tmax) )
		{
			return false;
		}
		if (tymin > tmin)
		{
			tmin = tymin;
		}
		if (tymax < tmax)
		{
			tmax = tymax;
		}
		if (ray.Direction.Z >= 0) 
		{
			tzmin = (Z - ray.Origin.Z) / ray.Direction.Z;
			tzmax = ((Z + Depth) - ray.Origin.Z) / ray.Direction.Z;
		}
		else 
		{
			tzmin = ((Z + Depth) - ray.Origin.Z) / ray.Direction.Z;
			tzmax = (Z - ray.Origin.Z) / ray.Direction.Z;
		}
		if ( (tmin > tzmax) || (tzmin > tmax) )
		{
			return false;
		}
		if (tzmin > tmin)
		{
			tmin = tzmin;
		}
		if (tzmax < tmax)
		{
			tmax = tzmax;
		}
		return true;//( (tmin < t1) && (tmax > t0) );
	}

	float X;
	float Y;
	float Z;
	float Width;
	float Height;
	float Depth;
private:
};