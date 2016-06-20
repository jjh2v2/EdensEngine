#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"

struct Direct3DResources
{
	Direct3DResources()
	{
		BufferCount = DEFAULT_BUFFERING_COUNT;
		CurrentBuffer = 0;
		Device = NULL;
		DXGIFactory = NULL;
		SwapChain = NULL;
		RTVHeap = NULL;
		RTVDescriptorSize = 0;
		CommandQueue = NULL;
		CommandList = NULL;
		Fence = NULL;
		FenceEvent = NULL;

		for (uint32 bufferIndex = 0; bufferIndex < BufferCount; bufferIndex++)
		{
			FenceValues.Add(0);
			BackBufferTargets.Add(NULL);
			CommandAllocators.Add(NULL);
		}
	}

	uint32 BufferCount;
	uint32 CurrentBuffer;
	ID3D12Device *Device;
	IDXGIFactory4 *DXGIFactory;
	IDXGISwapChain3	*SwapChain;
	DynamicArray<ID3D12Resource*> BackBufferTargets;
	ID3D12DescriptorHeap *RTVHeap;
	uint32 RTVDescriptorSize;
	ID3D12CommandQueue *CommandQueue;
	DynamicArray<ID3D12CommandAllocator*> CommandAllocators;
	ID3D12GraphicsCommandList *CommandList;
	D3D12_VIEWPORT ScreenViewport;
	ID3D12Fence* Fence;
	DynamicArray<uint64> FenceValues;
	HANDLE FenceEvent;
};