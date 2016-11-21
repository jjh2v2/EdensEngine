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

	return NULL;
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