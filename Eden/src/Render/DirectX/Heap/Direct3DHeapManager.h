#pragma once
#include "Render/DirectX/Heap/DescriptorHeap.h"
#include <map>

enum RenderPassDescriptorHeapType
{
    RenderPassDescriptorHeapType_GBuffer,
    RenderPassDescriptorHeapType_Shadows,
    RenderPassDescriptorHeapType_PostProcess
};

class Direct3DHeapManager
{
public:
	Direct3DHeapManager(ID3D12Device* device);
	~Direct3DHeapManager();

	DescriptorHeapHandle GetNewRTVDescriptorHeapHandle() { return mRTVDescriptorHeap->GetNewHeapHandle(); }
	DescriptorHeapHandle GetNewSRVDescriptorHeapHandle() { return mSRVDescriptorHeap->GetNewHeapHandle(); }
	DescriptorHeapHandle GetNewDSVDescriptorHeapHandle() { return mDSVDescriptorHeap->GetNewHeapHandle(); }
	DescriptorHeapHandle GetNewSamplerDescriptorHeapHandle() { return mSamplerDescriptorHeap->GetNewHeapHandle(); }

	void FreeRTVDescriptorHeapHandle(DescriptorHeapHandle handle) { mRTVDescriptorHeap->FreeHeapHandle(handle); }
	void FreeSRVDescriptorHeapHandle(DescriptorHeapHandle handle) { mSRVDescriptorHeap->FreeHeapHandle(handle); }
	void FreeDSVDescriptorHeapHandle(DescriptorHeapHandle handle) { mDSVDescriptorHeap->FreeHeapHandle(handle); }
	void FreeSamplerDescriptorHeapHandle(DescriptorHeapHandle handle) { mSamplerDescriptorHeap->FreeHeapHandle(handle); }

	DynamicDescriptorHeap *GetRTVDescriptorHeap() { return mRTVDescriptorHeap; }
	DynamicDescriptorHeap *GetSRVDescriptorHeap() { return mSRVDescriptorHeap; }
	DynamicDescriptorHeap *GetDSVDescriptorHeap() { return mDSVDescriptorHeap; }
	DynamicDescriptorHeap *GetSamplerDescriptorHeap() { return mSamplerDescriptorHeap; }

    RenderPassDescriptorHeap *GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType passType, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 frameIndex, uint32 numDescriptors, bool reset = true);

private:
    ID3D12Device* mDevice;

	DynamicDescriptorHeap *mRTVDescriptorHeap;
	DynamicDescriptorHeap *mSRVDescriptorHeap;
	DynamicDescriptorHeap *mDSVDescriptorHeap;
	DynamicDescriptorHeap *mSamplerDescriptorHeap;

    struct RenderPassDescriptorHeapGroup
    {
        RenderPassDescriptorHeapGroup()
        {
            memset(SRVHeap,     0, sizeof(RenderPassDescriptorHeap*) * FRAME_BUFFER_COUNT);
            memset(SamplerHeap, 0, sizeof(RenderPassDescriptorHeap*) * FRAME_BUFFER_COUNT);
        }

        RenderPassDescriptorHeap *SRVHeap[FRAME_BUFFER_COUNT];
        RenderPassDescriptorHeap *SamplerHeap[FRAME_BUFFER_COUNT];
    };

    std::map<RenderPassDescriptorHeapType, RenderPassDescriptorHeapGroup> mRenderPassDescriptorHeapGroups;
};