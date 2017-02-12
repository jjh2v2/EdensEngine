#pragma once
#include "Render/Shader/ShaderTechnique.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"
#include "Render/Buffer/GPUResource.h"
#include "Render/Texture/Texture.h"
#include "Render/DirectX/Heap/DescriptorHeap.h"
#include "Core/Containers/DynamicArray.h"

class Material
{
public:
	Material(ID3D12Device *device, ConstantBuffer *constantBuffer, DynamicArray<Texture*> &textures);
	~Material();

	void CommitConstantBufferChanges();
	MaterialBuffer *GetMaterialBuffer() { return &mMaterialBuffer; }

private:
	ShaderTechnique *mShaderTechnique;
	ConstantBuffer *mConstantBuffer;
	MaterialBuffer mMaterialBuffer;
	DynamicArray<Texture*> mTextures;
};