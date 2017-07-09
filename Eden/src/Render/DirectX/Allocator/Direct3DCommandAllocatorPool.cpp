#include "Render/DirectX/Allocator/Direct3DCommandAllocatorPool.h"

Direct3DCommandAllocatorPool::Direct3DCommandAllocatorPool(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE commandType, uint32 numAllocators)
{
    for (uint32 i = 0; i < numAllocators; i++)
    {
        ID3D12CommandAllocator *newAllocator;
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommandAllocator(commandType, IID_PPV_ARGS(&newAllocator)));
        newAllocator->SetName(L"Direct3DContext::mCommandAllocator");
        mAvailableAllocators.Add(newAllocator);
    }
}

Direct3DCommandAllocatorPool::~Direct3DCommandAllocatorPool()
{
    if (mReturnedAllocators.CurrentSize() > 0)
    {
        bool allAllocatorsShouldBeDone = false;
        Application::Assert(allAllocatorsShouldBeDone);
    }
    
    for (uint32 i = 0; i < mAvailableAllocators.CurrentSize(); i++)
    {
        mAvailableAllocators[i]->Release();
        mAvailableAllocators[i] = NULL;
    }
}

uint32 Direct3DCommandAllocatorPool::GetNumAllocatorsProcessing(uint64 mostRecentFence)
{
    CheckReturnedAllocatorState(mostRecentFence);
    return mReturnedAllocators.CurrentSize();
}

void Direct3DCommandAllocatorPool::CheckReturnedAllocatorState(uint64 mostRecentFence)
{
    for (int32 i = 0; i < (int32)mReturnedAllocators.CurrentSize(); i++)
    {
        if (mostRecentFence >= mReturnedAllocators[i].Fence)
        {
            mAvailableAllocators.Add(mReturnedAllocators[i].Allocator);
            mReturnedAllocators.Remove(i);
            i--;
        }
    }
}

ID3D12CommandAllocator *Direct3DCommandAllocatorPool::GetAllocator(uint64 mostRecentFence)
{
    CheckReturnedAllocatorState(mostRecentFence);

    if (mAvailableAllocators.CurrentSize() > 0)
    {
        return mAvailableAllocators.RemoveLast();
    }

    bool shouldIncreaseAllocatorPoolSize = false;
    Application::Assert(shouldIncreaseAllocatorPoolSize);

    return NULL;
}

void Direct3DCommandAllocatorPool::ReturnAllocator(ID3D12CommandAllocator *allocator, uint64 fence)
{
    ReturnedAllocator returnedAllocator;
    returnedAllocator.Allocator = allocator;
    returnedAllocator.Fence = fence;

    mReturnedAllocators.Add(returnedAllocator);
}