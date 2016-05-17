#pragma once
#include "Core/Platform/PlatformCore.h"

class DescriptorHeapHandle
{
public:
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() { return mCPUHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() { return mGPUHandle; }
	uint32 GetHeapIndex() { return mHeapIndex; }

	void SetCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) { mCPUHandle = cpuHandle; }
	void SetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) { mGPUHandle = gpuHandle; }
	void SetHeapIndex(uint32 heapIndex) { mHeapIndex = heapIndex; }

	bool IsValid() { return mCPUHandle.ptr != NULL; }
	bool IsReferencedByShader() { return mGPUHandle.ptr != NULL; }

private:
	D3D12_CPU_DESCRIPTOR_HANDLE mCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE mGPUHandle;
	uint32						mHeapIndex;
};