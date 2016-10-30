#pragma once

#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/Queue/Direct3DQueue.h"
#include <mutex>

class Direct3DQueue
{
public:
	Direct3DQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE commandType);
	~Direct3DQueue();

	uint64 IncrementFence();
	bool IsFenceComplete(uint64 fenceValue);
	void StallForFence(Direct3DQueue* otherQueue, uint64 fenceValue);
	void StallForQueue(Direct3DQueue* otherQueue);

	//void StallForProducer(CommandQueue& Producer);
	void WaitForFence(uint64 FenceValue);
	void WaitForIdle() { WaitForFence(mNextFenceValue - 1); }

	ID3D12CommandQueue* GetCommandQueue() { return mCommandQueue; }

	uint64 GetNextFenceValue() { return mNextFenceValue; }
	ID3D12Fence* GetFence() { return mFence; }
	uint64 GetNextFenceValue() { return mNextFenceValue; }

private:

	uint64 ExecuteCommandList(ID3D12CommandList* List);
	//ID3D12CommandAllocator* RequestAllocator();
	//void DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator);

private:
	ID3D12CommandQueue* mCommandQueue;
	D3D12_COMMAND_LIST_TYPE mQueueType;

	std::mutex mFenceMutex;
	std::mutex mEventMutex;

	// Lifetime of these objects is managed by the descriptor cache
	ID3D12Fence* mFence;
	uint64 mNextFenceValue;
	uint64 mLastCompletedFenceValue;
	HANDLE mFenceEventHandle;
};