#include "Render/DirectX/Heap/DescriptorHeap.h"

DescriptorHeap::DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 numDescriptors, bool isReferencedByShader)
{
	mHeapType = heapType;
	mMaxDescriptors = numDescriptors;
	mIsReferencedByShader = isReferencedByShader;

	mCurrentDescriptorIndex = 0;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.NumDescriptors = uint32(numDescriptors);
	heapDesc.Type = heapType;
	heapDesc.Flags = mIsReferencedByShader ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

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
	mFreeDescriptors.Clear();
	
	mDescriptorHeap->Release();
	mDescriptorHeap = NULL;
}

DescriptorHeapHandle DescriptorHeap::GetNewHeapHandle()
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
		Direct3DUtils::ThrowRuntimeError("Ran out of descriptor heap handles, need to increase heap size.");
	}

	DescriptorHeapHandle newHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mDescriptorHeapCPUStart;
	cpuHandle.ptr += newHandleID * mDescriptorSize;
	newHandle.SetCPUHandle(cpuHandle);

	if (mIsReferencedByShader)
	{

	}

	handle.CPUHandle = CPUStart;
	handle.CPUHandle.ptr += idx * DescriptorSize;
	if (ShaderVisible)
	{
		handle.GPUHandle = GPUStart;
		handle.GPUHandle.ptr += idx * DescriptorSize;
	}

	return handle;
}

void DescriptorHeap::FreeHeapHandle(DescriptorHeapHandle handle)
{

}