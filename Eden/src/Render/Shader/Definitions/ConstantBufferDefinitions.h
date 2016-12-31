#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Vector/Vector4.h"
#include "Core/Vector/Vector3.h"
#include "Core/Vector/Vector2.h"

struct CameraBuffer	//per frame 
{
	D3DXMATRIX viewMatrix;
	D3DXMATRIX projectionMatrix;
};

struct MaterialBuffer	//per object 
{
	D3DXMATRIX worldMatrix;
	Vector4 diffuseColor;
	Vector2 tiling;
	float roughness;
	float metalness;
	float materialIntensity;
	bool usesNormalMap;
	bool usesRoughMetalMap;
};