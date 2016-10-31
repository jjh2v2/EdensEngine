#include "Render/DirectX/Context/Direct3DContext.h"

Direct3DContext::Direct3DContext(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE commandType)
{
	mContextType = commandType;
	//m_CpuLinearAllocator(kCpuWritable);
	//m_GpuLinearAllocator(kGpuExclusive);
	mCommandList = NULL;
	mCommandAllocator = NULL;
	//ZeroMemory(m_CurrentDescriptorHeaps, sizeof(m_CurrentDescriptorHeaps));

	mCurrentGraphicsRootSignature = NULL;
	mCurrentGraphicsPipelineState = NULL;
	mCurrentComputeRootSignature = NULL;
	mCurrentComputePipelineState = NULL;
	mNumBarriersToFlush = 0;

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommandAllocator(commandType, IID_PPV_ARGS(&mCommandAllocator)));
	wchar_t AllocatorName[32];
	mCommandAllocator->SetName(L"Direct3DContext::mCommandAllocator");

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommandList(1, commandType, mCommandAllocator, NULL, IID_PPV_ARGS(&mCommandList)));
	mCommandList->SetName(L"Direct3DContext::mCommandList");

	for (uint32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
	{
		mCurrentDescriptorHeaps[i] = NULL;
	}
}

Direct3DContext::~Direct3DContext()
{
	if (mCommandList)
	{
		mCommandList->Release();
		mCommandList = NULL;
	}

	if (mCommandAllocator)
	{
		mCommandAllocator->Release();
		mCommandAllocator = NULL;
	}
}

uint64 Direct3DContext::Flush(Direct3DQueueManager *queueManager, bool waitForCompletion)
{
	FlushResourceBarriers();

	uint64 queueFence = queueManager->GetQueue(mContextType)->ExecuteCommandList(mCommandList);

	if (waitForCompletion)
	{
		queueManager->WaitForFence(queueFence);
	}

	// Reset the command list, keep the previous state
	mCommandList->Reset(mCommandAllocator, NULL);

	if (mCurrentGraphicsRootSignature)
	{
		mCommandList->SetGraphicsRootSignature(mCurrentGraphicsRootSignature);
		mCommandList->SetPipelineState(mCurrentGraphicsPipelineState);
	}
	if (mCurrentComputeRootSignature)
	{
		mCommandList->SetComputeRootSignature(mCurrentComputeRootSignature);
		mCommandList->SetPipelineState(mCurrentComputePipelineState);
	}

	BindDescriptorHeaps();

	return queueFence;
}

void Direct3DContext::FlushResourceBarriers()
{
	if (mNumBarriersToFlush > 0)
	{
		mCommandList->ResourceBarrier(mNumBarriersToFlush, mResourceBarrierBuffer);
		mNumBarriersToFlush = 0;
	}
}

void Direct3DContext::BindDescriptorHeaps()
{
	uint32 nonNullHeapCount = 0;
	ID3D12DescriptorHeap* heapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	for (uint32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
	{
		if (mCurrentDescriptorHeaps[i] != nullptr)
		{
			heapsToBind[nonNullHeapCount++] = mCurrentDescriptorHeaps[i];
		}
	}

	if (nonNullHeapCount > 0)
	{
		mCommandList->SetDescriptorHeaps(nonNullHeapCount, heapsToBind);
	}
}

void Direct3DContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap)
{
	if (mCurrentDescriptorHeaps[heapType] != heap)
	{
		mCurrentDescriptorHeaps[heapType] = heap;
		BindDescriptorHeaps();
	}
}

void Direct3DContext::SetDescriptorHeaps(uint32 numHeaps, D3D12_DESCRIPTOR_HEAP_TYPE heapTypes[], ID3D12DescriptorHeap* heaps[])
{
	bool changedHeap = false;

	for (uint32 i = 0; i < numHeaps; ++i)
	{
		if (mCurrentDescriptorHeaps[heapTypes[i]] != heaps[i])
		{
			mCurrentDescriptorHeaps[heapTypes[i]] = heaps[i];
			changedHeap = true;
		}
	}

	if (changedHeap)
	{
		BindDescriptorHeaps();
	}
}