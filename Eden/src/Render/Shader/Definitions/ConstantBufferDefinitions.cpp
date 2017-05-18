#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"

void MaterialBuffer::SetWorldMatrix(const D3DXMATRIX &matrix)
{
	//usually might use memcmp but with floating point precision, better to only update when the difference > Epsilon
	if (!MathHelper::FloatsAreEqual(matrix._11, mConstants.worldMatrix._11) ||
		!MathHelper::FloatsAreEqual(matrix._12, mConstants.worldMatrix._12) ||
		!MathHelper::FloatsAreEqual(matrix._13, mConstants.worldMatrix._13) ||
		!MathHelper::FloatsAreEqual(matrix._14, mConstants.worldMatrix._14) ||
		!MathHelper::FloatsAreEqual(matrix._21, mConstants.worldMatrix._21) ||
		!MathHelper::FloatsAreEqual(matrix._22, mConstants.worldMatrix._22) ||
		!MathHelper::FloatsAreEqual(matrix._23, mConstants.worldMatrix._23) ||
		!MathHelper::FloatsAreEqual(matrix._24, mConstants.worldMatrix._24) ||
		!MathHelper::FloatsAreEqual(matrix._31, mConstants.worldMatrix._31) ||
		!MathHelper::FloatsAreEqual(matrix._32, mConstants.worldMatrix._32) ||
		!MathHelper::FloatsAreEqual(matrix._33, mConstants.worldMatrix._33) ||
		!MathHelper::FloatsAreEqual(matrix._34, mConstants.worldMatrix._34) ||
		!MathHelper::FloatsAreEqual(matrix._41, mConstants.worldMatrix._41) ||
		!MathHelper::FloatsAreEqual(matrix._42, mConstants.worldMatrix._42) ||
		!MathHelper::FloatsAreEqual(matrix._43, mConstants.worldMatrix._43) ||
		!MathHelper::FloatsAreEqual(matrix._44, mConstants.worldMatrix._44))
	{
		mIsDirty = true;
		mConstants.worldMatrix = matrix;
	}
}

void MaterialBuffer::SetDiffuseColor(const Color &color)
{
	if (mConstants.diffuseColor != color)
	{
		mIsDirty = true;
		mConstants.diffuseColor = color;
	}
}

void MaterialBuffer::SetTiling(const Vector2 &tiling)
{
	if (mConstants.tiling != tiling)
	{
		mIsDirty = true;
		mConstants.tiling = tiling;
	}
}

void MaterialBuffer::SetRoughness(const float &roughness)
{
	if (!MathHelper::FloatsAreEqual(roughness, mConstants.roughness))
	{
		mIsDirty = true;
		mConstants.roughness = roughness;
	}
}

void MaterialBuffer::SetMetalness(const float &metalness)
{
	if (!MathHelper::FloatsAreEqual(metalness, mConstants.metalness))
	{
		mIsDirty = true;
		mConstants.metalness = metalness;
	}
}

void MaterialBuffer::SetMaterialIntensity(const float &materialIntensity)
{
	if (!MathHelper::FloatsAreEqual(materialIntensity, mConstants.materialIntensity))
	{
		mIsDirty = true;
		mConstants.materialIntensity = materialIntensity;
	}
}

void MaterialBuffer::SetUsesNormalMap(const bool &usesNormalMap)
{
	bool isUsingNormalMap = mConstants.usesNormalMap != 0;
	if (isUsingNormalMap != usesNormalMap)
	{
		mIsDirty = true;
		mConstants.usesNormalMap = usesNormalMap ? 1 : 0;
	}
}

void MaterialBuffer::SetUsesRoughmetalMap(const bool &usesRoughMetalMap)
{
	bool isUsingRoughMetal = mConstants.usesRoughMetalMap != 0;
	if (isUsingRoughMetal != usesRoughMetalMap)
	{
		mIsDirty = true;
		mConstants.usesRoughMetalMap = usesRoughMetalMap ? 1 : 0;
	}
}