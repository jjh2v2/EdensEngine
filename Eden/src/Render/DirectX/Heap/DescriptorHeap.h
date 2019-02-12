#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"
#include "Render/DirectX/Heap/DescriptorHeapHandle.h"

class DescriptorHeap
{
public:
    DescriptorHeap(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 numDescriptors, bool isReferencedByShader);
    virtual ~DescriptorHeap();

    ID3D12DescriptorHeap *GetHeap() { return mDescriptorHeap; }
    D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() { return mHeapType; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetHeapCPUStart() { return mDescriptorHeapCPUStart; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetHeapGPUStart() { return mDescriptorHeapGPUStart; }
    uint32 GetMaxDescriptors() { return mMaxDescriptors; }
    uint32 GetDescriptorSize() { return mDescriptorSize; }

protected:
    ID3D12DescriptorHeap *mDescriptorHeap;
    D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;
    D3D12_CPU_DESCRIPTOR_HANDLE mDescriptorHeapCPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE mDescriptorHeapGPUStart;
    uint32 mMaxDescriptors;
    uint32 mDescriptorSize;
    bool   mIsReferencedByShader;
};

class StaticDescriptorHeap : public DescriptorHeap
{
public:
    StaticDescriptorHeap(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 numDescriptors);
    ~StaticDescriptorHeap() final;

    DescriptorHeapHandle GetNewHeapHandle();
    void FreeHeapHandle(DescriptorHeapHandle handle);

private:
    DynamicArray<uint32> mFreeDescriptors;
    uint32 mCurrentDescriptorIndex;
    uint32 mActiveHandleCount;
};

class RenderPassDescriptorHeap : public DescriptorHeap
{
public:
    RenderPassDescriptorHeap(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 numDescriptors);
    ~RenderPassDescriptorHeap() final;

    void Reset();
    DescriptorHeapHandle GetHeapHandleBlock(uint32 count);

private:
    uint32 mCurrentDescriptorIndex;
};