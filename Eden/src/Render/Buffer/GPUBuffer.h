#pragma once

#include "Render/Buffer/GPUResource.h"

class GPUBuffer : public GPUResource
{
public:
	GPUBuffer(ID3D12Device *device, uint32 elementCount, uint32 elementSize, void* initData = NULL);
	virtual ~GPUBuffer();

	D3D12_CPU_DESCRIPTOR_HANDLE &GetUAV() { return mUnorderedAccessView; }
	D3D12_CPU_DESCRIPTOR_HANDLE &GetSRV() { return mShaderResourceView; }

protected:
	D3D12_CPU_DESCRIPTOR_HANDLE mUnorderedAccessView;
	D3D12_CPU_DESCRIPTOR_HANDLE mShaderResourceView;

	size_t mBufferSize;
	uint32 mElementCount;
	uint32 mElementSize;
	D3D12_RESOURCE_FLAGS mResourceFlags;
};