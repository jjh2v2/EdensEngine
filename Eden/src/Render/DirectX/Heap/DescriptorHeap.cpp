#include "Render/DirectX/Heap/DescriptorHeap.h"

DescriptorHeap::DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 numDescriptors, bool isReferencedByShader)
{
	mHeapType = heapType;
	mMaxDescriptors = numDescriptors;
	mIsReferencedByShader = isReferencedByShader;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.NumDescriptors = uint32(numDescriptors);
	heapDesc.Type = heapType;
	heapDesc.Flags = mIsReferencedByShader ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mDescriptorHeap)));
	mDescriptorHeapCPUStart = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	if (mIsReferencedByShader)
	{
		mDescriptorHeapGPUStart = mDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	}	

	mDescriptorSize = device->GetDescriptorHandleIncrementSize(heapType);
}

DescriptorHeap::~DescriptorHeap()
{
	mDescriptorHeap->Release();
	mDescriptorHeap = NULL;
}

DynamicDescriptorHeap::DynamicDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 numDescriptors, bool isReferencedByShader)
	:DescriptorHeap(device, heapType, numDescriptors, isReferencedByShader)
{
	mCurrentDescriptorIndex = 0;
	mActiveHandleCount = 0;
}

DynamicDescriptorHeap::~DynamicDescriptorHeap()
{
	if (mActiveHandleCount != 0)
	{
		Direct3DUtils::ThrowRuntimeError("There were active handles when the descriptor heap was destroyed. Look for leaks.");
	}

	mFreeDescriptors.Clear();
}

DescriptorHeapHandle DynamicDescriptorHeap::GetNewHeapHandle()
{
	uint32 newHandleID = 0;

	if (mCurrentDescriptorIndex < mMaxDescriptors)
	{
		newHandleID = mCurrentDescriptorIndex;
		mCurrentDescriptorIndex++;
	}
	else if (mFreeDescriptors.CurrentSize() > 0)
	{
		newHandleID = mFreeDescriptors.RemoveLast();
	}
	else
	{
		Direct3DUtils::ThrowRuntimeError("Ran out of dynamic descriptor heap handles, need to increase heap size.");
	}

	DescriptorHeapHandle newHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mDescriptorHeapCPUStart;
	cpuHandle.ptr += newHandleID * mDescriptorSize;
	newHandle.SetCPUHandle(cpuHandle);

	if (mIsReferencedByShader)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = mDescriptorHeapGPUStart;
		gpuHandle.ptr += newHandleID * mDescriptorSize;
		newHandle.SetGPUHandle(gpuHandle);
	}

	newHandle.SetHeapIndex(newHandleID);
	mActiveHandleCount++;

	return newHandle;
}

void DynamicDescriptorHeap::FreeHeapHandle(DescriptorHeapHandle handle)
{
	mFreeDescriptors.Add(handle.GetHeapIndex());

	if (mActiveHandleCount == 0)
	{
		Direct3DUtils::ThrowRuntimeError("Freeing heap handles when there should be none left");
	}
	mActiveHandleCount--;
}

void DynamicDescriptorHeap::Reset()
{
	mFreeDescriptors.Clear();
	mCurrentDescriptorIndex = 0;
	mActiveHandleCount = 0;
}

RenderPassDescriptorHeap::RenderPassDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 numDescriptors, bool isReferencedByShader)
	:DescriptorHeap(device, heapType, numDescriptors, isReferencedByShader)
{
	mCurrentDescriptorIndex = 0;
}

RenderPassDescriptorHeap::~RenderPassDescriptorHeap()
{

}

DescriptorHeapHandle RenderPassDescriptorHeap::GetHeapHandleBlock(uint32 count)
{
	uint32 newHandleID = 0;
	uint32 blockEnd = mCurrentDescriptorIndex + count;

	if (blockEnd < mMaxDescriptors)
	{
		newHandleID = mCurrentDescriptorIndex;
		mCurrentDescriptorIndex = blockEnd;
	}
	else
	{
		Direct3DUtils::ThrowRuntimeError("Ran out of render pass descriptor heap handles, need to increase heap size.");
	}

	DescriptorHeapHandle newHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mDescriptorHeapCPUStart;
	cpuHandle.ptr += newHandleID * mDescriptorSize;
	newHandle.SetCPUHandle(cpuHandle);

	if (mIsReferencedByShader)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = mDescriptorHeapGPUStart;
		gpuHandle.ptr += newHandleID * mDescriptorSize;
		newHandle.SetGPUHandle(gpuHandle);
	}

	newHandle.SetHeapIndex(newHandleID);

	return newHandle;
}

void RenderPassDescriptorHeap::Reset()
{
	mCurrentDescriptorIndex = 0;
}