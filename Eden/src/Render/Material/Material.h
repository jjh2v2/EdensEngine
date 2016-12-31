#pragma once
#include "Render/Shader/ShaderTechnique.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"
#include "Render/Buffer/GPUResource.h"
#include "Render/Texture/Texture.h"

class Material
{
public:
	Material(MaterialBuffer initialConstants, ConstantBuffer *constantBuffer);
	~Material();

	void AddTexture(Texture *texture) { mTextures.Add(texture); }
	void CommitMaterialChanges();
	MaterialBuffer *GetMaterialBuffer() { return &mConstants; }

private:
	ShaderTechnique *mShaderTechnique;
	ConstantBuffer *mConstantBuffer;
	MaterialBuffer mConstants;
	DynamicArray<Texture*> mTextures;
};