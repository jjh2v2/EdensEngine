#pragma once
#include "Core/Platform/PlatformCore.h"

class DescriptorHeapHandle
{
public:
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() { return mCPUHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() { return mGPUHandle; }

	bool IsValid() { return mCPUHandle.ptr != NULL; }
	bool IsShaderVisible() { return mGPUHandle.ptr != NULL; }

private:
	D3D12_CPU_DESCRIPTOR_HANDLE mCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE mGPUHandle;
};