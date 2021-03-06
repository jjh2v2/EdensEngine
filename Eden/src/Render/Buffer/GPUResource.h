#pragma once

#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/Heap/DescriptorHeapHandle.h"
#include "Core/Containers/DynamicArray.h"

class GPUResource
{
public:
    GPUResource(ID3D12Resource *resource, D3D12_RESOURCE_STATES usageState);
    virtual ~GPUResource();

    ID3D12Resource *GetResource() { return mResource; }
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() { return mGPUAddress; }
    D3D12_RESOURCE_STATES GetUsageState() { return mUsageState; }
    void SetUsageState(D3D12_RESOURCE_STATES usageState) { mUsageState = usageState; }

    bool GetIsReady() { return mIsReady; }
    void SetIsReady(bool isReady) { mIsReady = isReady; }

protected:
    ID3D12Resource *mResource;
    D3D12_GPU_VIRTUAL_ADDRESS mGPUAddress;
    D3D12_RESOURCE_STATES mUsageState;
    bool mIsReady;
};


class TextureResource : public GPUResource
{
public:
    TextureResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, DescriptorHeapHandle shaderResourceViewHandle);
    ~TextureResource() override;

    DescriptorHeapHandle GetShaderResourceViewHandle() { return mShaderResourceViewHandle; }

private:
    DescriptorHeapHandle mShaderResourceViewHandle;
};


class BackBufferTarget : public GPUResource
{
public:
    BackBufferTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, DescriptorHeapHandle renderTargetViewHandle);
    ~BackBufferTarget() override;

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
        DescriptorHeapHandle BaseHandle;
        DynamicArray<DescriptorHeapHandle> Handles;
    };

    RenderTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, D3D12_RESOURCE_DESC renderTargetDesc, DescriptorHeapHandle renderTargetViewHandle, 
        DynamicArray<DescriptorHeapHandle> &renderTargetViewArray, DescriptorHeapHandle shaderResourceViewHandle, const UAVHandle &unorderedAccessView);
    ~RenderTarget() override;

    DescriptorHeapHandle GetRenderTargetViewHandle() { return mRenderTargetViewHandle; }
    DescriptorHeapHandle GetRenderTargetViewHandle(uint32 index) { return mRenderTargetViewArray[index]; }
    const DynamicArray<DescriptorHeapHandle> &GetRenderTargetViewArray() { return mRenderTargetViewArray; }
    DescriptorHeapHandle GetShaderResourceViewHandle() { return mShaderResourceViewHandle; }
    DescriptorHeapHandle GetUnorderedAccessViewHandle(uint32 mipIndex, uint32 arrayIndex = 0);
    DescriptorHeapHandle GetUnorderedAccessViewHandle() { return mUnorderedAccessView.BaseHandle; }

    bool GetHasUAV() { return mUnorderedAccessView.HasUAV; }
    uint16 GetArraySize() { return mRenderTargetDesc.DepthOrArraySize; }
    uint16 GetMipCount() { return mRenderTargetDesc.MipLevels; }
    uint32 GetWidth() { return (uint32)mRenderTargetDesc.Width; }
    uint32 GetHeight() { return (uint32)mRenderTargetDesc.Height; }

private:
    D3D12_RESOURCE_DESC	 mRenderTargetDesc;
    DescriptorHeapHandle mRenderTargetViewHandle;
    DescriptorHeapHandle mShaderResourceViewHandle;
    UAVHandle mUnorderedAccessView;
    DynamicArray<DescriptorHeapHandle> mRenderTargetViewArray;
};

class DepthStencilTarget : public GPUResource
{
public:
    DepthStencilTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, D3D12_RESOURCE_DESC depthStencilDesc, DescriptorHeapHandle depthStencilViewHandle, DescriptorHeapHandle readOnlyDepthStencilViewHandle,
        DynamicArray<DescriptorHeapHandle> &depthStencilViewArray, DynamicArray<DescriptorHeapHandle> &readOnlyDepthStencilViewArray, DescriptorHeapHandle shaderResourceViewHandle);
    ~DepthStencilTarget() override;

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
    VertexBuffer(ID3D12Resource *resource, D3D12_RESOURCE_STATES usageState, uint32 vertexStride, uint32 bufferSize);
    ~VertexBuffer() override;

    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() { return mVertexBufferView; }

private:
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
};


class IndexBuffer : public GPUResource
{
public:
    IndexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32 bufferSize);
    ~IndexBuffer() override;

    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() { return mIndexBufferView; }

private:
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
};


class ConstantBuffer : public GPUResource
{
public:
    ConstantBuffer(ID3D12Resource *resource, D3D12_RESOURCE_STATES usageState, uint32 bufferSize, DescriptorHeapHandle constantBufferViewHandle);
    ~ConstantBuffer() override;

    void SetConstantBufferData(const void *bufferData, uint32 bufferSize);
    DescriptorHeapHandle GetConstantBufferViewHandle() { return mConstantBufferViewHandle; }

private:
    void *mMappedBuffer;
    uint32 mBufferSize;
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
    ~StructuredBuffer() override;

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

class FilteredCubeMapRenderTexture : public GPUResource
{
public:
    FilteredCubeMapRenderTexture(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32 numMipLevels, const DynamicArray<DescriptorHeapHandle> &uavHandles, DescriptorHeapHandle srvHandle);
    ~FilteredCubeMapRenderTexture() override;

    DescriptorHeapHandle GetShaderResourceViewHandle() { return mSRVHandle; }
    DescriptorHeapHandle GetUnorderedAccessViewHandle(uint32 mipIndex, uint32 arrayIndex = 0);
    uint32 GetMipCount() { return mNumMips; }

    void SetComputeFence(uint64 computeFence) { mComputeFence = computeFence; }
    uint64 GetComputeFence() { return mComputeFence; }

private:
    uint64 mComputeFence;
    uint32 mNumMips;
    DynamicArray<DescriptorHeapHandle> mUAVHandles;
    DescriptorHeapHandle mSRVHandle;
};

class RayTraceBuffer : public GPUResource
{
public:
    enum RayTraceBufferType
    {
        RayTraceBufferType_Acceleration_Structure = 0,
        RayTraceBufferType_Instancing,
        RayTraceBufferType_Shader_Binding_Table_Storage,
        RayTraceBufferType_Transform
    };

    RayTraceBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, RayTraceBufferType bufferType, DescriptorHeapHandle srvHandle);
    ~RayTraceBuffer() override;
    void MapInstanceDescData(const void *instanceDescData, uint32 numInstanceDescs);
    void MapTransform(const void *transform, uint32 sizeOfTransform);
    DescriptorHeapHandle GetShaderResourceViewHandle() { return mSRVHandle; }

private:
    RayTraceBufferType mBufferType;
    DescriptorHeapHandle mSRVHandle;
};