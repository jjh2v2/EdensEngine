#pragma once

#include "Render/DirectX/Context/Direct3DContext.h"
#include "Render/DirectX/Queue/Direct3DQueueManager.h"
#include "Render/DirectX/Heap/Direct3DHeapManager.h"
class Direct3DContextManager
{
public:
	//TDA: Create a second Graphics context to start recording while the previous context is being consumed
	Direct3DContextManager(ID3D12Device* device);
	~Direct3DContextManager();

	Direct3DHeapManager *GetHeapManager() { return mHeapManager; }

	Direct3DQueueManager *GetQueueManager() { return mQueueManager; }
	GraphicsContext *GetGraphicsContext() { return mGraphicsContext; }
	UploadContext *GetUploadContext() { return mUploadContext; }

	VertexBuffer *CreateVertexBuffer(void* vertexData, uint32 vertexStride, uint32 bufferSize);
	IndexBuffer *CreateIndexBuffer(void* indexData, uint32 bufferSize);
	ConstantBuffer *CreateConstantBuffer(uint32 bufferSize);

private:
	ID3D12Device *mDevice;
	Direct3DHeapManager *mHeapManager;
	Direct3DQueueManager *mQueueManager;
	GraphicsContext *mGraphicsContext;
	UploadContext *mUploadContext;
};