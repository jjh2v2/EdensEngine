#include "Render/DirectX/Context/Direct3DContext.h"

Direct3DContext::Direct3DContext(D3D12_COMMAND_LIST_TYPE commandType)
{
	mContextType = commandType;
	//m_CpuLinearAllocator(kCpuWritable);
	//m_GpuLinearAllocator(kGpuExclusive);
	mCommandList = NULL;
	mCurrentAllocator = NULL;
	//ZeroMemory(m_CurrentDescriptorHeaps, sizeof(m_CurrentDescriptorHeaps));

	mCurrentGraphicsRootSignature = nullptr;
	mCurrentGraphicsPipelineState = nullptr;
	mCurrentComputeRootSignature = nullptr;
	mCurrentComputePipelineState = nullptr;
	mNumBarriersToFlush = 0;
}

Direct3DContext::~Direct3DContext()
{
	if (mCommandList)
	{
		mCommandList->Release();
		mCommandList = NULL;
	}
}