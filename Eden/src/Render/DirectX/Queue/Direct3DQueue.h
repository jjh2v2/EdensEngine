#pragma once

#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/Queue/Direct3DQueue.h"
#include <mutex>

class Direct3DQueue
{
public:
    Direct3DQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE commandType);
    ~Direct3DQueue();

    bool IsFenceComplete(uint64 fenceValue);
    void InsertWait(uint64 fenceValue);
    void InsertWaitForQueueFence(Direct3DQueue* otherQueue, uint64 fenceValue);
    void InsertWaitForQueue(Direct3DQueue* otherQueue);

    void WaitForFenceCPUBlocking(uint64 fenceValue);
    void WaitForIdle() { WaitForFenceCPUBlocking(mNextFenceValue - 1); }

    ID3D12CommandQueue* GetCommandQueue() { return mCommandQueue; }

    uint64 PollCurrentFenceValue();
    uint64 GetLastCompletedFence() { return mLastCompletedFenceValue; }
    uint64 GetNextFenceValue() { return mNextFenceValue; }
    ID3D12Fence* GetFence() { return mFence; }

    uint64 ExecuteCommandList(ID3D12CommandList* List);

private:
    ID3D12CommandQueue* mCommandQueue;
    D3D12_COMMAND_LIST_TYPE mQueueType;

    std::mutex mFenceMutex;
    std::mutex mEventMutex;

    ID3D12Fence* mFence;
    uint64 mNextFenceValue;
    uint64 mLastCompletedFenceValue;
    HANDLE mFenceEventHandle;
};