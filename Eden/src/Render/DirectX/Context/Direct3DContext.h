#pragma once

#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/Queue/Direct3DQueueManager.h"

class Direct3DContext
{
public:
	Direct3DContext(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE commandType);
	~Direct3DContext();

	Direct3DContext(const Direct3DContext&) = delete;
	Direct3DContext & operator=(const Direct3DContext&) = delete;

	uint64 Flush(Direct3DQueueManager *queueManager, bool waitForCompletion = false);
	void FlushResourceBarriers();
	void BindDescriptorHeaps();

	ID3D12GraphicsCommandList *GetCommandList(){ return mCommandList; }

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);
	void SetDescriptorHeaps(uint32 numHeaps, D3D12_DESCRIPTOR_HEAP_TYPE heapTypes[], ID3D12DescriptorHeap* heaps[]);

private:
	D3D12_COMMAND_LIST_TYPE mContextType;
	ID3D12GraphicsCommandList* mCommandList;
	ID3D12CommandAllocator* mCommandAllocator;

	ID3D12RootSignature* mCurrentGraphicsRootSignature;
	ID3D12PipelineState* mCurrentGraphicsPipelineState;
	ID3D12RootSignature* mCurrentComputeRootSignature;
	ID3D12PipelineState* mCurrentComputePipelineState;

	D3D12_RESOURCE_BARRIER mResourceBarrierBuffer[16];
	uint32 mNumBarriersToFlush;

	ID3D12DescriptorHeap* mCurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	//LinearAllocator m_CpuLinearAllocator;
	//LinearAllocator m_GpuLinearAllocator;
};