#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"
#include "Render/DirectX/Heap/DescriptorHeapHandle.h"

class DescriptorHeap
{
public:
	DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 numDescriptors, bool isReferencedByShader);
	~DescriptorHeap();

	DescriptorHeapHandle GetNewHeapHandle();
	void FreeHeapHandle(DescriptorHeapHandle handle);

private:
	ID3D12DescriptorHeap* mDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;
	D3D12_CPU_DESCRIPTOR_HANDLE mDescriptorHeapCPUStart;
	D3D12_GPU_DESCRIPTOR_HANDLE mDescriptorHeapGPUStart;

	DynamicArray<uint32> mFreeDescriptors;

	uint32 mCurrentDescriptorIndex;
	uint32 mMaxDescriptors;

	uint32 mDescriptorSize;
	bool   mIsReferencedByShader;
};