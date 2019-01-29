#pragma once

#include "Render/DirectX/Queue/Direct3DQueue.h"

class Direct3DQueueManager
{
public:
	Direct3DQueueManager(ID3D12Device* device);
	~Direct3DQueueManager();

	Direct3DQueue *GetGraphicsQueue() { return mGraphicsQueue; }
	Direct3DQueue *GetComputeQueue() { return mComputeQueue; }
	Direct3DQueue *GetCopyQueue() { return mCopyQueue; }

	Direct3DQueue *GetQueue(D3D12_COMMAND_LIST_TYPE commandType);

	bool IsFenceComplete(uint64 fenceValue);
	void WaitForFenceCPUBlocking(uint64 fenceValue);
	void WaitForAllIdle();

private:
	Direct3DQueue *mGraphicsQueue;
	Direct3DQueue *mComputeQueue;
	Direct3DQueue *mCopyQueue;
};