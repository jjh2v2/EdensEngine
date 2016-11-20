#include "Render/Buffer/GPUResource.h"

GPUResource::GPUResource()
{
	mResource = NULL;
	mGPUAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
	mUsageState = D3D12_RESOURCE_STATE_COMMON;
	mTransitioningState = D3D12_GPU_RESOURCE_STATE_UNKNOWN;
}

GPUResource::GPUResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState)
{
	mResource = resource;
	mUsageState = usageState;
	mGPUAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
	mTransitioningState = D3D12_GPU_RESOURCE_STATE_UNKNOWN;
}

GPUResource::~GPUResource()
{
	mResource->Release();
	mResource = NULL;
}