#include "Render/Material/Material.h"

Material::Material(ID3D12Device *device, ConstantBuffer *constantBuffer, DynamicArray<Texture*> &textures)
{
	mConstantBuffer = constantBuffer;
	
	for (uint32 i = 0; i < textures.CurrentSize(); i++)
	{
		mTextures.Add(textures[i]);
	}
}

Material::~Material()
{
	delete mConstantBuffer;
}

void Material::CommitConstantBufferChanges()
{
	if (mMaterialBuffer.GetIsDirty())
	{
		mConstantBuffer->SetConstantBufferData(&mMaterialBuffer.GetMaterialConstants(), sizeof(MaterialConstants));
		mMaterialBuffer.SetIsDirty(false);
	}
}