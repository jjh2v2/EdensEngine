#pragma once

#include "Render/DirectX/Context/Direct3DContext.h"

class Direct3DContextManager
{
public:
	Direct3DContextManager();
	~Direct3DContextManager();
private:
	void InitializeBuffer(ID3D12Device* device, GPUResource *resource, const void* initData, size_t numBytes, bool useOffset = false, size_t offset = 0);
	//void InitializeTexture(GPUResource& destination, UINT numSubresources, D3D12_SUBRESOURCE_DATA subresourceData[]);
	//void InitializeTextureArraySlice(GPUResource& destination, UINT sliceIndex, GPUResource& source);
};