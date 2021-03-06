#pragma once
#include "Core/Vector/Vector3.h"
#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/Heap/DescriptorHeapHandle.h"
#include "Render/Buffer/GPUResource.h"

class Texture
{
public:
	Texture();
	~Texture();

	void SetTextureResource(TextureResource *resource) { mTextureResource = resource; }
	TextureResource *GetTextureResource() { return mTextureResource; }

	void SetFormat(DXGI_FORMAT format) { mFormat = format; }
	DXGI_FORMAT GetFormat() { return mFormat; }
	void SetDimensions(Vector3 dimensions) { mDimensions = dimensions; }
	Vector3 GetDimensions() { return mDimensions; }
	void SetMipCount(uint32 mipCount) { mMipCount = mipCount; }
	uint32 GetMipCount() { return mMipCount; }
	void SetArraySize(uint32 arraySize) { mArraySize = arraySize; }
	uint32 GetArraySize() { return mArraySize; }
	void SetIsCubeMap(bool isCubeMap) { mIsCubeMap = isCubeMap; }
	bool GetIsCubeMap() { return mIsCubeMap; }

    bool GetIsReady() { return mTextureResource && mTextureResource->GetIsReady(); }
    void SetIsReady(bool isReady) { mTextureResource->SetIsReady(isReady); }

private:
	TextureResource* mTextureResource;

	DXGI_FORMAT mFormat;
	Vector3		mDimensions;
	uint32		mMipCount;
	uint32		mArraySize;
	bool		mIsCubeMap;
};