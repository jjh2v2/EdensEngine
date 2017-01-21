#pragma once
#include "Render/DirectX/Heap/DescriptorHeap.h"

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

private:
	DynamicDescriptorHeap *mRTVDescriptorHeap;
	DynamicDescriptorHeap *mSRVDescriptorHeap;
	DynamicDescriptorHeap *mDSVDescriptorHeap;
	DynamicDescriptorHeap *mSamplerDescriptorHeap;
};