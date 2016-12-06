#include "Render/Buffer/GPUResource.h"

GPUResource::GPUResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, bool isTexture)
{
	mResource = resource;
	mUsageState = usageState;

	//MSDN: GetGPUVirtualAddress is only useful for buffer resources, it will return zero for all texture resources.
	if (isTexture)
	{
		mGPUAddress = 0;
	}
	else
	{
		mGPUAddress = resource->GetGPUVirtualAddress();	
	}
	
	mTransitioningState = D3D12_GPU_RESOURCE_STATE_UNKNOWN;
}

GPUResource::~GPUResource()						//Sometimes we assign the resource from somewhere else, and don't want it released. How do we handle it?
{
//	mResource->Release();
//	mResource = NULL;
}