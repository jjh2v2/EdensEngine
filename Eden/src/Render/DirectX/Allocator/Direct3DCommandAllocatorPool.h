#pragma once

#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"

class Direct3DCommandAllocatorPool
{
public:
    Direct3DCommandAllocatorPool(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE commandType, uint32 numAllocators);
    ~Direct3DCommandAllocatorPool();

    uint32 GetNumAllocatorsProcessing(uint64 mostRecentFence);
    ID3D12CommandAllocator *GetAllocator(uint64 mostRecentFence);
    void ReturnAllocator(ID3D12CommandAllocator *allocator, uint64 fence);

private:
    void CheckReturnedAllocatorState(uint64 mostRecentFence);

    struct ReturnedAllocator
    {
        uint64 Fence;
        ID3D12CommandAllocator *Allocator;
    };

    uint32 mNumAllocators;
    DynamicArray<ID3D12CommandAllocator*> mAvailableAllocators;
    DynamicArray<ReturnedAllocator> mReturnedAllocators;
};