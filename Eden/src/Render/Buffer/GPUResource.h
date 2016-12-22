#pragma once

#include "Core/Platform/PlatformCore.h"

class GPUResource
{
public:
	GPUResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState);
	virtual ~GPUResource();

	ID3D12Resource* GetResource() { return mResource; }
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() { return mGPUAddress; }
	D3D12_RESOURCE_STATES GetUsageState() { return mUsageState; }
	D3D12_RESOURCE_STATES GetTransitionState() { return mTransitioningState; }
	void SetUsageState(D3D12_RESOURCE_STATES usageState) { mUsageState = usageState; }
	void SetTransitionState(D3D12_RESOURCE_STATES transitionState) { mTransitioningState = transitionState; }

protected:
	ID3D12Resource *mResource;
	D3D12_GPU_VIRTUAL_ADDRESS mGPUAddress;
	D3D12_RESOURCE_STATES mUsageState;
	D3D12_RESOURCE_STATES mTransitioningState;
};

class TextureResource : public GPUResource
{
public:
	TextureResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState);
	virtual ~TextureResource();
};

class RenderTarget : public GPUResource
{
public:
	RenderTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState);
	virtual ~RenderTarget();
};

class VertexBuffer : public GPUResource
{
public:
	VertexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32 vertexStride, uint32 bufferSize);
	virtual ~VertexBuffer();

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() { return mVertexBufferView; }

protected:
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
};

class IndexBuffer : public GPUResource
{
public:
	IndexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32 bufferSize);
	virtual ~IndexBuffer();

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() { return mIndexBufferView; }

private:
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
};