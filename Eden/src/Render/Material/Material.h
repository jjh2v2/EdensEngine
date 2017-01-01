#pragma once
#include "Render/Shader/ShaderTechnique.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"
#include "Render/Buffer/GPUResource.h"
#include "Render/Texture/Texture.h"
#include "Render/DirectX/Heap/DescriptorHeap.h"

class Material
{
public:
	Material(ID3D12Device *device, MaterialBuffer initialConstants, ConstantBuffer *constantBuffer, DynamicArray<Texture*> &textures);
	~Material();

	void CommitConstantBufferChanges();
	void CommitTextureChanges(ID3D12Device *device);
	MaterialBuffer *GetMaterialBuffer() { return &mConstants; }

private:
	ShaderTechnique *mShaderTechnique;
	ConstantBuffer *mConstantBuffer;
	MaterialBuffer mConstants;
	DynamicArray<Texture*> mTextures;
	DescriptorHeap *mSRVHeap;
	DescriptorHeap *mSamplerHeap;
};