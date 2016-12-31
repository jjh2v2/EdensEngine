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
	DescriptorHeapHandle GetNewSamplerDescriptorHeapHandle() { return mRTVDescriptorHeap->GetNewHeapHandle(); }

	void FreeRTVDescriptorHeapHandle(DescriptorHeapHandle handle) { mRTVDescriptorHeap->FreeHeapHandle(handle); }
	void FreeSRVDescriptorHeapHandle(DescriptorHeapHandle handle) { mSRVDescriptorHeap->FreeHeapHandle(handle); }
	void FreeDSVDescriptorHeapHandle(DescriptorHeapHandle handle) { mDSVDescriptorHeap->FreeHeapHandle(handle); }
	void FreeSamplerDescriptorHeapHandle(DescriptorHeapHandle handle) { mRTVDescriptorHeap->FreeHeapHandle(handle); }

	DescriptorHeap *GetRTVDescriptorHeap() { return mRTVDescriptorHeap; }
	DescriptorHeap *GetSRVDescriptorHeap() { return mSRVDescriptorHeap; }
	DescriptorHeap *GetDSVDescriptorHeap() { return mDSVDescriptorHeap; }
	DescriptorHeap *GetSamplerDescriptorHeap() { return mSamplerDescriptorHeap; }

private:
	DescriptorHeap *mRTVDescriptorHeap;
	DescriptorHeap *mSRVDescriptorHeap;
	DescriptorHeap *mDSVDescriptorHeap;
	DescriptorHeap *mSamplerDescriptorHeap;
};