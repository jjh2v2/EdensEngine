#pragma once

#include "Core/Platform/PlatformCore.h"

class Direct3DContext
{
public:
	Direct3DContext(D3D12_COMMAND_LIST_TYPE commandType);
	~Direct3DContext();

private:
	D3D12_COMMAND_LIST_TYPE mContextType;
	ID3D12GraphicsCommandList* mCommandList;
	ID3D12CommandAllocator* mCurrentAllocator;

	ID3D12RootSignature* mCurrentGraphicsRootSignature;
	ID3D12PipelineState* mCurrentGraphicsPipelineState;
	ID3D12RootSignature* mCurrentComputeRootSignature;
	ID3D12PipelineState* mCurrentComputePipelineState;

	D3D12_RESOURCE_BARRIER mResourceBarrierBuffer[16];
	uint32 mNumBarriersToFlush;

	//ID3D12DescriptorHeap* mCurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	//LinearAllocator m_CpuLinearAllocator;
	//LinearAllocator m_GpuLinearAllocator;
};