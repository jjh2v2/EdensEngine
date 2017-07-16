#pragma once
#include "Render/Shader/ShaderTechnique.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"
#include "Render/Buffer/GPUResource.h"
#include "Render/Texture/Texture.h"
#include "Render/Texture/Sampler/Sampler.h"
#include "Render/DirectX/Heap/DescriptorHeap.h"
#include "Core/Containers/DynamicArray.h"
#include "Render/Shader/Definitions/MaterialDefinitions.h"

class RenderPassContext;

class Material
{
public:
	Material(ConstantBuffer *constantBuffer[FRAME_BUFFER_COUNT]);
	~Material();

	void SetTexture(MaterialTextureType textureType, Texture *texture);
	
	ConstantBuffer *GetConstantBuffer(uint32 frameIndex) { return mConstantBuffer[frameIndex]; }
	MaterialBuffer *GetMaterialBuffer() { return &mMaterialBuffer; }
	void ApplyMaterial(RenderPassContext *renderPassContext);

private:
    void CommitConstantBufferChanges(uint32 frameIndex);

	ConstantBuffer *mConstantBuffer[FRAME_BUFFER_COUNT];
	MaterialBuffer mMaterialBuffer;
	Texture* mTextures[MaterialTextureType_Max];
    uint32 mDirtyCheckCount;
};