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
	Material(ConstantBuffer *constantBuffer);
	~Material();

	void SetTexture(MaterialTextureType textureType, Texture *texture);
	void CommitConstantBufferChanges();
	ConstantBuffer *GetConstantBuffer() { return mConstantBuffer; }
	MaterialBuffer *GetMaterialBuffer() { return &mMaterialBuffer; }
	void ApplyMaterial(RenderPassContext *renderPassContext);

private:
	ConstantBuffer *mConstantBuffer;
	MaterialBuffer mMaterialBuffer;
	Texture* mTextures[MaterialTextureType_Max];
};