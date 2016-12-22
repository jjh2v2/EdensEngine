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

TextureResource::TextureResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState)
	:GPUResource(resource, usageState)
{

}

TextureResource::~TextureResource()
{

}

RenderTarget::RenderTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState)
	:GPUResource(resource, usageState)
{

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