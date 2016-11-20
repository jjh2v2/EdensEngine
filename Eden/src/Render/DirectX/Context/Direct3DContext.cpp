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

void Direct3DContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap *heap)
{
	if (mCurrentDescriptorHeaps[heapType] != heap)
	{
		mCurrentDescriptorHeaps[heapType] = heap;
		BindDescriptorHeaps();
	}
}

void Direct3DContext::SetDescriptorHeaps(uint32 numHeaps, D3D12_DESCRIPTOR_HEAP_TYPE heapTypes[], ID3D12DescriptorHeap *heaps[])
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

void GraphicsContext::SetRootSignature(const RootSignatureInfo &rootSignature)
{
	if (rootSignature.RootSignature == mCurrentComputeRootSignature)
	{
		return;
	}

	mCurrentComputeRootSignature = rootSignature.RootSignature;
	mCommandList->SetGraphicsRootSignature(mCurrentComputeRootSignature);
	//m_DynamicDescriptorHeap.ParseGraphicsRootSignature(RootSig);
}

void GraphicsContext::SetRenderTargets(uint32 numRenderTargets, const D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[])
{
	mCommandList->OMSetRenderTargets(numRenderTargets, renderTargets, FALSE, NULL);
}

void GraphicsContext::SetRenderTargets(uint32 numRenderTargets, const D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[], D3D12_CPU_DESCRIPTOR_HANDLE depthStencil)
{
	mCommandList->OMSetRenderTargets(numRenderTargets, renderTargets, FALSE, &depthStencil);
}

void GraphicsContext::SetViewport(const D3D12_VIEWPORT &viewPort)
{
	mCommandList->RSSetViewports(1, &viewPort);
}

void GraphicsContext::SetViewport(float x, float y, float w, float h, float minDepth /* = 0.0f */, float maxDepth /* = 1.0f */)
{
	D3D12_VIEWPORT viewPort;
	viewPort.Width = w;
	viewPort.Height = h;
	viewPort.MinDepth = minDepth;
	viewPort.MaxDepth = maxDepth;
	viewPort.TopLeftX = x;
	viewPort.TopLeftY = y;

	mCommandList->RSSetViewports(1, &viewPort);
}

void GraphicsContext::SetScissorRect(const D3D12_RECT &rect)
{
	mCommandList->RSSetScissorRects(1, &rect);
}

void GraphicsContext::SetScissorRect(uint32 left, uint32 top, uint32 right, uint32 bottom)
{
	SetScissorRect(CD3DX12_RECT(left, top, right, bottom));
}

void GraphicsContext::SetStencilRef(uint32 stencilRef)
{
	mCommandList->OMSetStencilRef(stencilRef);
}

void GraphicsContext::SetBlendFactor(Color blendFactor)
{
	float color[4] = { blendFactor.R, blendFactor.G, blendFactor.B, blendFactor.A };
	mCommandList->OMSetBlendFactor(color);
}

void GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
	mCommandList->IASetPrimitiveTopology(topology);
}

void GraphicsContext::SetPipelineState(ShaderPSO &pipeline)
{
	if (mCurrentGraphicsPipelineState == pipeline.GetPipelineState())
	{
		return;
	}

	mCurrentGraphicsPipelineState = pipeline.GetPipelineState();
	mCommandList->SetPipelineState(mCurrentGraphicsPipelineState);
}

void GraphicsContext::SetConstants(uint32 index, uint32 numConstants, const void *bufferData)
{
	mCommandList->SetGraphicsRoot32BitConstants(index, numConstants, bufferData, 0);
}

void GraphicsContext::SetConstantBuffer(uint32 index, D3D12_GPU_VIRTUAL_ADDRESS constantBuffer)
{
	mCommandList->SetComputeRootConstantBufferView(index, constantBuffer);
}

void GraphicsContext::SetShaderResourceView(uint32 index, GPUBuffer &shaderResourceView, uint64 offset /* = 0 */)
{
	mCommandList->SetGraphicsRootShaderResourceView(index, shaderResourceView.GetGpuAddress() + offset);
}

void GraphicsContext::SetUnorderedAccessView(uint32 index, GPUBuffer &unorderedAccessView, uint64 offset /* = 0 */)
{
	mCommandList->SetComputeRootUnorderedAccessView(index, unorderedAccessView.GetGpuAddress() + offset);
}

void GraphicsContext::SetDescriptorTable(uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
	mCommandList->SetGraphicsRootDescriptorTable(index, handle);
}

void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW &indexBuffer)
{
	mCommandList->IASetIndexBuffer(&indexBuffer);
}

void GraphicsContext::SetVertexBuffer(uint32 slot, const D3D12_VERTEX_BUFFER_VIEW &vertexBuffer)
{
	SetVertexBuffers(slot, 1, &vertexBuffer);
}

void GraphicsContext::SetVertexBuffers(uint32 slot, uint32 count, const D3D12_VERTEX_BUFFER_VIEW vertexBuffers[])
{
	mCommandList->IASetVertexBuffers(slot, count, vertexBuffers);
}

void GraphicsContext::Draw(uint32 vertexCount, uint32 vertexStartOffset /* = 0 */)
{

}

void GraphicsContext::DrawIndexed(uint32 indexCount, uint32 startIndexLocation /* = 0 */, int32 baseVertexLocation /* = 0 */)
{

}

void GraphicsContext::DrawInstanced(uint32 vertexCountPerInstance, uint32 instanceCount, uint32 startVertexLocation /* = 0 */, uint32 startInstanceLocation /* = 0 */)
{

}

void GraphicsContext::DrawIndexedInstanced(uint32 indexCountPerInstance, uint32 instanceCount, uint32 startIndexLocation, int32 baseVertexLocation, uint32 startInstanceLocation)
{

}

void GraphicsContext::DrawIndirect(GPUBuffer& argumentBuffer, size_t argumentBufferOffset /* = 0 */)
{

}