#pragma once
#include "Render/DirectX/Heap/DescriptorHeap.h"

class Direct3DHeapManager
{
public:
	Direct3DHeapManager(ID3D12Device* device);
	~Direct3DHeapManager();

private:
	DescriptorHeap *mRTVDescriptorHeap;
	DescriptorHeap *mSRVDescriptoHeap;
	DescriptorHeap *mDSVDescriptoHeap;
	DescriptorHeap *mSamplerDescriptorHeap;
};