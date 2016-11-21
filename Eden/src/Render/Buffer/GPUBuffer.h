#pragma once

#include "Render/Buffer/GPUResource.h"

//this is unfinished, UAV/SRV not implemented, this is intended for compute buffers

class GPUBuffer : public GPUResource
{
public:
	GPUBuffer();
	virtual ~GPUBuffer();

	D3D12_CPU_DESCRIPTOR_HANDLE &GetUAV() { return mUnorderedAccessView; }
	D3D12_CPU_DESCRIPTOR_HANDLE &GetSRV() { return mShaderResourceView; }

	void SetBufferInfo(size_t bufferSize, uint32 elementCount, uint32 elementSize, D3D12_RESOURCE_FLAGS flags);
	D3D12_RESOURCE_FLAGS GetResourceFlags() { return mResourceFlags; }
	size_t GetBufferSize() { return mBufferSize; }

protected:
	D3D12_CPU_DESCRIPTOR_HANDLE mUnorderedAccessView;
	D3D12_CPU_DESCRIPTOR_HANDLE mShaderResourceView;

	size_t mBufferSize;
	uint32 mElementCount;
	uint32 mElementSize;
	D3D12_RESOURCE_FLAGS mResourceFlags;
};