#pragma once
#include "Render/DirectX/Heap/DescriptorHeap.h"
#include <map>

enum RenderPassDescriptorHeapType
{
    RenderPassDescriptorHeapType_GBuffer,           //TDA: will be faster when we get rid of most of these, but for simplicity while developing, it works
    RenderPassDescriptorHeapType_Sky,
    RenderPassDescriptorHeapType_ShadowCompute,
    RenderPassDescriptorHeapType_ShadowRender,
    RenderPassDescriptorHeapType_Lighting,
    RenderPassDescriptorHeapType_PostProcess,
    RenderPassDescriptorHeapType_TextureProcessing,
    RenderPassDescriptorHeapType_RayShadow,
    RenderPassDescriptorHeapType_Water
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

	StaticDescriptorHeap *GetRTVDescriptorHeap() { return mRTVDescriptorHeap; }
	StaticDescriptorHeap *GetSRVDescriptorHeap() { return mSRVDescriptorHeap; }
	StaticDescriptorHeap *GetDSVDescriptorHeap() { return mDSVDescriptorHeap; }
	StaticDescriptorHeap *GetSamplerDescriptorHeap() { return mSamplerDescriptorHeap; }

    RenderPassDescriptorHeap *GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType passType, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 frameIndex, uint32 numDescriptors, bool reset = true);

private:
    ID3D12Device* mDevice;

	StaticDescriptorHeap *mRTVDescriptorHeap;
	StaticDescriptorHeap *mSRVDescriptorHeap;
	StaticDescriptorHeap *mDSVDescriptorHeap;
	StaticDescriptorHeap *mSamplerDescriptorHeap;

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