#include "Render/Buffer/GPUResource.h"

GPUResource::GPUResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState)
{
	mResource = resource;
	mUsageState = usageState;
	//MSDN: GetGPUVirtualAddress is only used for buffer resources, it will return zero for all texture resources. (and also throw errors)
	mGPUAddress = 0; //other buffer types set this themselves
	
	mTransitioningState = D3D12_GPU_RESOURCE_STATE_UNKNOWN;
}

GPUResource::~GPUResource()						//Sometimes we assign the resource from somewhere else, and don't want it released. How do we handle it? Answer: we release it here, everything that creates a resource should do it through this
{
	mResource->Release();
	mResource = NULL;
}

TextureResource::TextureResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, DescriptorHeapHandle shaderResourceViewHandle)
	:GPUResource(resource, usageState)
{
	mShaderResourceViewHandle = shaderResourceViewHandle;
}

TextureResource::~TextureResource()
{

}

BackBufferTarget::BackBufferTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, DescriptorHeapHandle renderTargetViewHandle)
	:GPUResource(resource, usageState)
{
	mRenderTargetViewHandle = renderTargetViewHandle;
}

BackBufferTarget::~BackBufferTarget()
{

}

RenderTarget::RenderTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, D3D12_RESOURCE_DESC renderTargetDesc, DescriptorHeapHandle renderTargetViewHandle,
	DynamicArray<DescriptorHeapHandle> &renderTargetViewArray, DescriptorHeapHandle shaderResourceViewHandle, UAVHandle unorderedAccessViewHandle)
	:GPUResource(resource, usageState)
{
	mRenderTargetDesc = renderTargetDesc;
	mRenderTargetViewHandle = renderTargetViewHandle;
	mShaderResourceViewHandle = shaderResourceViewHandle;
	mUnorderedAccessViewHandle = unorderedAccessViewHandle;

	for (uint32 i = 0; i < renderTargetViewArray.CurrentSize(); i++)
	{
		mRenderTargetViewArray.Add(renderTargetViewArray[i]);
	}
}

RenderTarget::~RenderTarget()
{

}

VertexBuffer::VertexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32 vertexStride, uint32 bufferSize)
	:GPUResource(resource, usageState)
{
	mGPUAddress = resource->GetGPUVirtualAddress();
	mVertexBufferView.StrideInBytes = vertexStride;
	mVertexBufferView.SizeInBytes = bufferSize;
	mVertexBufferView.BufferLocation = mGPUAddress;
}

VertexBuffer::~VertexBuffer()
{

}

IndexBuffer::IndexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32 bufferSize)
	:GPUResource(resource, usageState)
{
	mGPUAddress = resource->GetGPUVirtualAddress();
	mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	mIndexBufferView.SizeInBytes = bufferSize;
	mIndexBufferView.BufferLocation = mGPUAddress;
}

IndexBuffer::~IndexBuffer()
{

}

ConstantBuffer::ConstantBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferDesc, DescriptorHeapHandle constantBufferViewHandle)
	:GPUResource(resource, usageState)
{
	mGPUAddress = constantBufferDesc.BufferLocation;
	mConstantBufferViewDesc = constantBufferDesc;
	mConstantBufferViewHandle = constantBufferViewHandle;

	mMappedBuffer = NULL;
	mResource->Map(0, NULL, &mMappedBuffer);
}

ConstantBuffer::~ConstantBuffer()
{

}

void ConstantBuffer::SetConstantBufferData(void* bufferData, uint32 bufferSize)
{
	Application::Assert(bufferSize < mConstantBufferViewDesc.SizeInBytes);
	memcpy(mMappedBuffer, bufferData, bufferSize);
}