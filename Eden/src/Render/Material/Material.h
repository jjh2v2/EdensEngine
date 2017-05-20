#pragma once
#include "Render/Shader/ShaderTechnique.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"
#include "Render/Buffer/GPUResource.h"
#include "Render/Texture/Texture.h"
#include "Render/Texture/Sampler/Sampler.h"
#include "Render/DirectX/Heap/DescriptorHeap.h"
#include "Core/Containers/DynamicArray.h"

class RenderPassContext;

class Material
{
public:
	Material(ConstantBuffer *constantBuffer, DynamicArray<Texture*> &textures);
	~Material();

	void CommitConstantBufferChanges();
	MaterialBuffer *GetMaterialBuffer() { return &mMaterialBuffer; }
	void ApplyMaterial(RenderPassContext *renderPassContext);

private:
	ConstantBuffer *mConstantBuffer;
	MaterialBuffer mMaterialBuffer;
	DynamicArray<Texture*> mTextures;
};