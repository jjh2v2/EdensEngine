#include "Render/Material/Material.h"

Material::Material(ID3D12Device *device, MaterialBuffer initialConstants, ConstantBuffer *constantBuffer, DynamicArray<Texture*> &textures)
{
	mConstants = initialConstants;
	mConstantBuffer = constantBuffer;
	mSRVHeap = new DescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, textures.CurrentSize(), true);
	mSamplerHeap = new DescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, textures.CurrentSize(), true);
	
	for (uint32 i = 0; i < textures.CurrentSize(); i++)
	{
		mTextures.Add(textures[i]);
	}

	CommitConstantBufferChanges();
	CommitTextureChanges(device);
}

Material::~Material()
{
	delete mConstantBuffer;
	delete mSRVHeap;
	delete mSamplerHeap;
}

void Material::CommitConstantBufferChanges()
{
	mConstantBuffer->SetConstantBufferData(mConstantBuffer, sizeof(ConstantBuffer));
}

void Material::CommitTextureChanges(ID3D12Device *device)
{
	D3D12_CPU_DESCRIPTOR_HANDLE srvHeapHandle = mSRVHeap->GetHeapCPUStart(); 

	for (uint32 i = 0; i < mTextures.CurrentSize(); i++)
	{
		device->CopyDescriptorsSimple(1, srvHeapHandle, mTextures[i]->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		srvHeapHandle.ptr += mSRVHeap->GetDescriptorSize();
	}
}