#pragma once

#include "Core/Platform/PlatformCore.h"

class GPUResource
{
public:
	GPUResource();
	GPUResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState);
	virtual ~GPUResource();

	ID3D12Resource* GetResource() { return mResource; }
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() { return mGPUAddress; }

protected:
	ID3D12Resource *mResource;
	D3D12_GPU_VIRTUAL_ADDRESS mGPUAddress;
	D3D12_RESOURCE_STATES mUsageState;
	D3D12_RESOURCE_STATES mTransitioningState;
};