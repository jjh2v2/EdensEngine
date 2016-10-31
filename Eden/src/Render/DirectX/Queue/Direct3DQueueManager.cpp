#include "Render/DirectX/Queue/Direct3DQueueManager.h"
#include "Render/Buffer/GPUResource.h"

Direct3DQueueManager::Direct3DQueueManager(ID3D12Device* device)
{
	mGraphicsQueue = new Direct3DQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	mComputeQueue = new Direct3DQueue(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	mCopyQueue = new Direct3DQueue(device, D3D12_COMMAND_LIST_TYPE_COPY);
}

Direct3DQueueManager::~Direct3DQueueManager()
{
	delete mGraphicsQueue;
	delete mComputeQueue;
	delete mCopyQueue;
}

Direct3DQueue *Direct3DQueueManager::GetQueue(D3D12_COMMAND_LIST_TYPE commandType)
{
	switch (commandType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return mGraphicsQueue;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE: 
		return mComputeQueue;
	case D3D12_COMMAND_LIST_TYPE_COPY: 
		return mCopyQueue;
	default: 
		Direct3DUtils::ThrowRuntimeError("Bad command type lookup in queue manager.");
	}
}

bool Direct3DQueueManager::IsFenceComplete(uint64 fenceValue)
{
	return GetQueue(D3D12_COMMAND_LIST_TYPE(fenceValue >> 56))->IsFenceComplete(fenceValue);
}

void Direct3DQueueManager::WaitForFence(uint64 fenceValue)
{
	Direct3DQueue *commandQueue = GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
	commandQueue->WaitForFence(fenceValue);
}

void Direct3DQueueManager::WaitForAllIdle()
{
	mGraphicsQueue->WaitForIdle();
	mComputeQueue->WaitForIdle();
	mCopyQueue->WaitForIdle();
}

//this should be in a context manager
void Direct3DQueueManager::InitializeBuffer(ID3D12Device* device, GPUResource *resource, const void* initData, size_t numBytes, bool useOffset, size_t offset)
{
	ID3D12Resource* UploadBuffer;

	CommandContext& InitContext = CommandContext::Begin();

	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = numBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&UploadBuffer)));

	void* DestAddress;
	UploadBuffer->Map(0, NULL, &DestAddress);
	SIMDMemCopy(DestAddress, initData, Math::DivideByMultiple(numBytes, 16));
	UploadBuffer->Unmap(0, NULL);

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	InitContext.TransitionResource(resource, D3D12_RESOURCE_STATE_COPY_DEST, true);
	if (useOffset)
	{
		InitContext.m_CommandList->CopyBufferRegion(resource->GetResource(), offset, UploadBuffer, 0, numBytes);
	}
	else
	{
		InitContext.m_CommandList->CopyResource(resource->GetResource(), UploadBuffer);
	}

	InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	InitContext.Finish(true);

	UploadBuffer->Release();
}