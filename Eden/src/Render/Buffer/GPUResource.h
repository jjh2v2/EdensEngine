#pragma once

#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/Heap/DescriptorHeapHandle.h"
#include "Core/Containers/DynamicArray.h"

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

    bool GetIsReady() { return mIsReady; }
    void SetIsReady(bool isReady) { mIsReady = isReady; }

protected:
	ID3D12Resource *mResource;
	D3D12_GPU_VIRTUAL_ADDRESS mGPUAddress;
	D3D12_RESOURCE_STATES mUsageState;
	D3D12_RESOURCE_STATES mTransitioningState;
    bool mIsReady;
};


class TextureResource : public GPUResource
{
public:
	TextureResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, DescriptorHeapHandle shaderResourceViewHandle);
	virtual ~TextureResource();

	DescriptorHeapHandle GetShaderResourceViewHandle() { return mShaderResourceViewHandle; }

private:
	DescriptorHeapHandle mShaderResourceViewHandle;
};


class BackBufferTarget : public GPUResource
{
public:
	BackBufferTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, DescriptorHeapHandle renderTargetViewHandle);
	virtual ~BackBufferTarget();

	DescriptorHeapHandle GetRenderTargetViewHandle() { return mRenderTargetViewHandle; }

private:
	DescriptorHeapHandle mRenderTargetViewHandle;
};

class RenderTarget : public GPUResource
{
public:
	struct UAVHandle
	{
		UAVHandle()
		{
			HasUAV = false;
		}

		bool HasUAV;
		DescriptorHeapHandle Handle;
	};

	RenderTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, D3D12_RESOURCE_DESC renderTargetDesc, DescriptorHeapHandle renderTargetViewHandle, 
		DynamicArray<DescriptorHeapHandle> &renderTargetViewArray, DescriptorHeapHandle shaderResourceViewHandle, UAVHandle unorderedAccessViewHandle);
	virtual ~RenderTarget();

	DescriptorHeapHandle GetRenderTargetViewHandle() { return mRenderTargetViewHandle; }
	DescriptorHeapHandle GetRenderTargetViewHandle(uint32 index) { return mRenderTargetViewArray[index]; }
	const DynamicArray<DescriptorHeapHandle> &GetRenderTargetViewArray() { return mRenderTargetViewArray; }
	DescriptorHeapHandle GetShaderResourceViewHandle() { return mShaderResourceViewHandle; }
	UAVHandle GetUnorderedAccessViewHandle() { return mUnorderedAccessViewHandle; }
	uint16 GetArraySize() { return mRenderTargetDesc.DepthOrArraySize; }

private:
	D3D12_RESOURCE_DESC	 mRenderTargetDesc;
	DescriptorHeapHandle mRenderTargetViewHandle;
	DescriptorHeapHandle mShaderResourceViewHandle;
	UAVHandle mUnorderedAccessViewHandle;
	DynamicArray<DescriptorHeapHandle> mRenderTargetViewArray;
};

class DepthStencilTarget : public GPUResource
{
public:
	DepthStencilTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, D3D12_RESOURCE_DESC depthStencilDesc, DescriptorHeapHandle depthStencilViewHandle, DescriptorHeapHandle readOnlyDepthStencilViewHandle,
		DynamicArray<DescriptorHeapHandle> &depthStencilViewArray, DynamicArray<DescriptorHeapHandle> &readOnlyDepthStencilViewArray, DescriptorHeapHandle shaderResourceViewHandle);
	virtual ~DepthStencilTarget();

	DescriptorHeapHandle GetDepthStencilViewHandle() { return mDepthStencilViewHandle; }
	DescriptorHeapHandle GetReadOnlyDepthStencilViewHandle() { return mReadOnlyDepthStencilViewHandle; }
	DescriptorHeapHandle GetDepthStencilViewHandle(uint32 index) { return mDepthStencilViewArray[index]; }
	DescriptorHeapHandle GetReadOnlyDepthStencilViewHandle(uint32 index) { return mReadOnlyDepthStencilViewArray[index]; }
	const DynamicArray<DescriptorHeapHandle> &GetDepthStencilViewArray() { return mDepthStencilViewArray; }
	DescriptorHeapHandle GetShaderResourceViewHandle() { return mShaderResourceViewHandle; }
	uint16 GetArraySize() { return mDepthStencilDesc.DepthOrArraySize; }

private:
	D3D12_RESOURCE_DESC	 mDepthStencilDesc;
	DescriptorHeapHandle mDepthStencilViewHandle;
	DescriptorHeapHandle mReadOnlyDepthStencilViewHandle;
	DescriptorHeapHandle mShaderResourceViewHandle;
	DynamicArray<DescriptorHeapHandle> mDepthStencilViewArray;
	DynamicArray<DescriptorHeapHandle> mReadOnlyDepthStencilViewArray;
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


class ConstantBuffer : public GPUResource
{
public:
	ConstantBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferDesc, DescriptorHeapHandle constantBufferViewHandle);
	virtual ~ConstantBuffer();

	void SetConstantBufferData(const void* bufferData, uint32 bufferSize);
	DescriptorHeapHandle GetConstantBufferViewHandle() { return mConstantBufferViewHandle; }

private:
	void *mMappedBuffer;
	D3D12_CONSTANT_BUFFER_VIEW_DESC mConstantBufferViewDesc;
	DescriptorHeapHandle mConstantBufferViewHandle;
};


enum StructuredBufferAccess
{
    GPU_READ_WRITE = 0,
    CPU_WRITE_GPU_READ = 1,
    CPU_WRITE_GPU_READ_WRITE = 2
};

class StructuredBuffer : public GPUResource
{
public:
    StructuredBuffer(ID3D12Resource* resource, ID3D12Resource *uploadResource, D3D12_RESOURCE_STATES usageState, bool isRaw, StructuredBufferAccess accessType, D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc, DescriptorHeapHandle uavHandle, DescriptorHeapHandle srvHandle);
    virtual ~StructuredBuffer();

    bool CopyToBuffer(const void* bufferData, uint32 bufferSize); //returns true if a GPU copy needs to be scheduled
    DescriptorHeapHandle GetUnorderedAccessViewHandle() { return mUnorderedAccessViewHandle; }
    DescriptorHeapHandle GetShaderResourceViewHandle() { return mShaderResourceViewHandle; }

private:
    void *mMappedBuffer;
    ID3D12Resource *mUploadResource;

    bool mIsRaw;
    StructuredBufferAccess mAccessType;
    D3D12_UNORDERED_ACCESS_VIEW_DESC mUAVDesc;
    DescriptorHeapHandle mUnorderedAccessViewHandle;
    DescriptorHeapHandle mShaderResourceViewHandle;
};