#include "Render/Material/Material.h"
#include "Render/DirectX/Context/Direct3DContext.h"
#include "Render/Texture/TextureManager.h"

Material::Material(ConstantBuffer *constantBuffer[FRAME_BUFFER_COUNT])
{
    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mConstantBuffer[i] = constantBuffer[i];
        mIsBufferDirty[i] = false;
    }
	
	memset(mTextures, NULL, sizeof(mTextures));
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

    if (wasMaterialBufferDirty)
    {
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            mIsBufferDirty[i] = true;
        }

        mMaterialBuffer.SetIsDirty(false);
    }

	if (mIsBufferDirty[frameIndex])
	{
		mConstantBuffer[frameIndex]->SetConstantBufferData(&mMaterialBuffer.GetMaterialConstants(), sizeof(MaterialConstants));
        mIsBufferDirty[frameIndex] = false;
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

ShadowMaterial::ShadowMaterial(ConstantBuffer *constantBuffer[FRAME_BUFFER_COUNT])
{
    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mConstantBuffer[i] = constantBuffer[i];
        mIsBufferDirty[i] = false;
    }
}

ShadowMaterial::~ShadowMaterial()
{

}

void ShadowMaterial::CommitConstantBufferChanges(uint32 frameIndex)
{
    bool wasMaterialBufferDirty = mMaterialBuffer.GetIsDirty();

    if (wasMaterialBufferDirty)
    {
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            mIsBufferDirty[i] = true;
        }

        mMaterialBuffer.SetIsDirty(false);
    }

    if (mIsBufferDirty[frameIndex])
    {
        mConstantBuffer[frameIndex]->SetConstantBufferData(&mMaterialBuffer.GetLightWorldViewProjMatrix(), sizeof(D3DXMATRIX));
        mIsBufferDirty[frameIndex] = false;
    }
}

void ShadowMaterial::ApplyMaterial(RenderPassContext *renderPassContext)
{
    GraphicsContext *graphicsContext = renderPassContext->GetGraphicsContext();
    RenderPassDescriptorHeap *cbvHeap = renderPassContext->GetCBVSRVHeap();
    DescriptorHeapHandle cbvHandle = cbvHeap->GetHeapHandleBlock(1);

    CommitConstantBufferChanges(renderPassContext->GetFrameIndex());
    graphicsContext->CopyDescriptors(1, cbvHandle.GetCPUHandle(), mConstantBuffer[renderPassContext->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetDescriptorTable(0, cbvHandle.GetGPUHandle());
}