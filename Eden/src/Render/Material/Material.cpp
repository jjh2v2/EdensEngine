#include "Render/Material/Material.h"
#include "Render/DirectX/Context/Direct3DContext.h"
#include "Render/Texture/TextureManager.h"

Material::Material(ConstantBuffer *constantBuffer[FRAME_BUFFER_COUNT])
{
    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mConstantBuffer[i] = constantBuffer[i];
    }
	
	memset(mTextures, NULL, sizeof(mTextures));
    mDirtyCheckCount = 0;
}

Material::~Material()
{
	
}

void Material::SetTexture(MaterialTextureType textureType, Texture *texture)
{
	mTextures[textureType] = texture;
}

void Material::CommitConstantBufferChanges(uint32 frameIndex)
{
    bool wasMaterialBufferDirty = mMaterialBuffer.GetIsDirty();

	if (wasMaterialBufferDirty || mDirtyCheckCount > 0)
	{
		mConstantBuffer[frameIndex]->SetConstantBufferData(&mMaterialBuffer.GetMaterialConstants(), sizeof(MaterialConstants));
	}

    if (wasMaterialBufferDirty)
    {
        mMaterialBuffer.SetIsDirty(false);
        mDirtyCheckCount = FRAME_BUFFER_COUNT - 1;
    }
    else if(mDirtyCheckCount > 0)
    {
        mDirtyCheckCount--;
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

    CommitConstantBufferChanges(renderPassContext->GetFrameIndex());
	graphicsContext->CopyDescriptors(1, cbvHandle.GetCPUHandle(), mConstantBuffer[renderPassContext->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (uint32 i = 0; i < renderPassTextureCount; i++)
	{
		MaterialTextureType textureType = renderPassContext->GetRenderPassTextureType(i);
        Texture *texture = mTextures[textureType];

		if (!texture || !texture->GetIsReady())
		{
            texture = TextureManager::GetDefaultTexture();
		}

        graphicsContext->CopyDescriptors(1, currentTextureHandle, texture->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		currentTextureHandle.ptr += cbvHeap->GetDescriptorSize();
	}

	graphicsContext->SetDescriptorTable(0, textureHandle.GetGPUHandle());
	graphicsContext->SetDescriptorTable(1, cbvHandle.GetGPUHandle());
}