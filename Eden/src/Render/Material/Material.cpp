#include "Render/Material/Material.h"
#include "Render/DirectX/Context/Direct3DContext.h"

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

void Material::ApplyMaterial(RenderPassContext *renderPassContext)
{
	GraphicsContext *graphicsContext = renderPassContext->GetGraphicsContext();
	RenderPassDescriptorHeap *cbvHeap = renderPassContext->GetCBVHeap();
	ShaderPSO *shaderPSO = mShaderTechnique->GetShader(renderPassContext->GetShaderPipelinePermutation());
	uint32 numTextures = mTextures.CurrentSize();

	DescriptorHeapHandle cbvHandle = cbvHeap->GetHeapHandleBlock(1);
	DescriptorHeapHandle textureHandle = cbvHeap->GetHeapHandleBlock(numTextures);
	D3D12_CPU_DESCRIPTOR_HANDLE currentTextureHandle = textureHandle.GetCPUHandle();

	for (uint32 i = 0; i < numTextures; i++)
	{
		graphicsContext->CopyDescriptors(1, currentTextureHandle, mTextures[i]->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		currentTextureHandle.ptr += cbvHeap->GetDescriptorSize();
	}

	graphicsContext->SetDescriptorTable(0, textureHandle.GetGPUHandle());
	graphicsContext->SetDescriptorTable(1, cbvHandle.GetGPUHandle());
	graphicsContext->SetPipelineState(shaderPSO);
	graphicsContext->SetRootSignature(shaderPSO->GetRootSignature());
}