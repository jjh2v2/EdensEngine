#pragma once

#include "Render/DirectX/Context/Direct3DContext.h"
#include "Render/DirectX/Queue/Direct3DQueueManager.h"

class Direct3DContextManager
{
public:
	//TDA: Create a second Graphics context to start recording while the previous context is being consumed
	Direct3DContextManager(ID3D12Device* device);
	~Direct3DContextManager();

	Direct3DQueueManager *GetQueueManager() { return mQueueManager; }
	GraphicsContext *GetGraphicsContext() { return mGraphicsContext; }
	UploadContext *GetUploadContext() { return mUploadContext; }

private:
	//GPUBuffer *CreateGPUBuffer(ID3D12Device* device, uint32 elementCount, uint32 elementSize, 
	//	const void* initData, size_t numBytes, bool useOffset = false, size_t offset = 0);
	//void InitializeTexture(GPUResource& destination, UINT numSubresources, D3D12_SUBRESOURCE_DATA subresourceData[]);
	//void InitializeTextureArraySlice(GPUResource& destination, UINT sliceIndex, GPUResource& source);

	Direct3DQueueManager *mQueueManager;
	GraphicsContext *mGraphicsContext;
	UploadContext *mUploadContext;
};