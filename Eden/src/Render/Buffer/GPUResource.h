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
	D3D12_RESOURCE_STATES GetUsageState() { return mUsageState; }
	D3D12_RESOURCE_STATES GetTransitionState() { return mTransitioningState; }
	void SetUsageState(D3D12_RESOURCE_STATES usageState) { mUsageState = usageState; }
	void SetTransitionState(D3D12_RESOURCE_STATES transitionState) { mTransitioningState = transitionState; }

protected:
	ID3D12Resource *mResource;
	D3D12_GPU_VIRTUAL_ADDRESS mGPUAddress;
	D3D12_RESOURCE_STATES mUsageState;
	D3D12_RESOURCE_STATES mTransitioningState;
};