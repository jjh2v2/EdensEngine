#include "Render/Material/Material.h"
#include "Render/DirectX/Context/Direct3DContext.h"

Material::Material(ConstantBuffer *constantBuffer)
{
	mConstantBuffer = constantBuffer;
	memset(mTextures, NULL, sizeof(mTextures));
}

Material::~Material()
{
	
}

void Material::SetTexture(MaterialTextureType textureType, Texture *texture)
{
	mTextures[textureType] = texture;
}

void Material::CommitConstantBufferChanges()
{
	if (mMaterialBuffer.GetIsDirty())
	{
		mConstantBuffer->SetConstantBufferData(&mMaterialBuffer.GetMaterialConstants(), sizeof(MaterialConstants));
		mMaterialBuffer.SetIsDirty(false);
	}
}

void Material::ApplyMaterial(RenderPassContext *renderPassContext)
{
	GraphicsContext *graphicsContext = renderPassContext->GetGraphicsContext();
	RenderPassDescriptorHeap *cbvHeap = renderPassContext->GetCBVSRVHeap();

	uint32 renderPassTextureCount = renderPassContext->GetRenderPassTextureCount();
	DescriptorHeapHandle cbvHandle = cbvHeap->GetHeapHandleBlock(1);
	DescriptorHeapHandle textureHandle = cbvHeap->GetHeapHandleBlock(renderPassTextureCount);
	D3D12_CPU_DESCRIPTOR_HANDLE currentTextureHandle = textureHandle.GetCPUHandle();

	graphicsContext->CopyDescriptors(1, cbvHandle.GetCPUHandle(), mConstantBuffer->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (uint32 i = 0; i < renderPassTextureCount; i++)
	{
		MaterialTextureType textureType = renderPassContext->GetRenderPassTextureType(i);

		if (mTextures[textureType])
		{
			graphicsContext->CopyDescriptors(1, currentTextureHandle, mTextures[textureType]->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		
		currentTextureHandle.ptr += cbvHeap->GetDescriptorSize();
	}

	graphicsContext->SetDescriptorTable(0, textureHandle.GetGPUHandle());
	graphicsContext->SetDescriptorTable(1, cbvHandle.GetGPUHandle());
}