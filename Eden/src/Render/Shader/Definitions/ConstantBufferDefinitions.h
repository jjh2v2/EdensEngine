#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Vector/Vector4.h"
#include "Core/Vector/Vector3.h"
#include "Core/Vector/Vector2.h"
#include "Core/Misc/Color.h"

struct CameraBuffer	//per frame 
{
	D3DXMATRIX viewMatrix;
	D3DXMATRIX projectionMatrix;
};

struct MaterialConstants
{
	MaterialConstants()
	{
		D3DXMatrixIdentity(&worldMatrix);
		diffuseColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
		tiling = Vector2(1.0f, 1.0f);
		roughness = 1.0f;
		metalness = 0.0f;
		materialIntensity = 1.0f;
		usesNormalMap = 0;
		usesRoughMetalMap = 0;
	}

	D3DXMATRIX worldMatrix;
	Color diffuseColor;
	Vector2 tiling;
	float roughness;
	float metalness;
	float materialIntensity;
	uint32 usesNormalMap;
	uint32 usesRoughMetalMap;
};

struct MaterialBuffer	//per object 
{
	MaterialBuffer()
	{
		mIsDirty = false;
	}

	bool GetIsDirty() { return mIsDirty; }
	void SetIsDirty(const bool &isDirty) { mIsDirty = isDirty; }

	const MaterialConstants &GetMaterialConstants() { return mConstants; }

	void SetWorldMatrix(const D3DXMATRIX &matrix);
	D3DXMATRIX GetWorldMatrix() { return mConstants.worldMatrix; }

	void SetDiffuseColor(const Color &color);
	Color GetDiffuseColor() { return mConstants.diffuseColor; }

	void SetTiling(const Vector2 &tiling);
	Vector2 GetTiling() { return mConstants.tiling; }

	void SetRoughness(const float &roughness);
	float GetRoughness() { return mConstants.roughness; }

	void SetMetalness(const float &metalness);
	float GetMetalness() { return mConstants.metalness; }

	void SetMaterialIntensity(const float &materialIntensity);
	float GetMaterialIntensity() { return mConstants.materialIntensity; }

	void SetUsesNormalMap(const bool &usesNormalMap);
	bool GetUsesNormalMap() { return mConstants.usesNormalMap != 0; }

	void SetUsesRoughmetalMap(const bool &usesRoughMetalMap);
	bool GetUsesRoughmetalMap() { return mConstants.usesRoughMetalMap != 0; }

private:
	bool mIsDirty;
	MaterialConstants mConstants;
};