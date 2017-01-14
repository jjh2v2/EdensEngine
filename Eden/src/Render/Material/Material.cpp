#include "Render/Material/Material.h"

Material::Material(ID3D12Device *device, MaterialBuffer initialConstants, ConstantBuffer *constantBuffer, DynamicArray<Texture*> &textures)
{
	mConstants = initialConstants;
	mConstantBuffer = constantBuffer;
	
	for (uint32 i = 0; i < textures.CurrentSize(); i++)
	{
		mTextures.Add(textures[i]);
	}

	CommitConstantBufferChanges();
}

Material::~Material()
{
	delete mConstantBuffer;
}

void Material::CommitConstantBufferChanges()
{
	mConstantBuffer->SetConstantBufferData(mConstantBuffer, sizeof(ConstantBuffer));
}