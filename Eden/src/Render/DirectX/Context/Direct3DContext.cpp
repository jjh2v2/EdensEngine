#include "Render/DirectX/Context/Direct3DContext.h"

Direct3DContext::Direct3DContext(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE commandType)
{
	mDevice = device;
	mContextType = commandType;
	mCommandList = NULL;
	mCommandAllocator = NULL;

	mCurrentGraphicsRootSignature = NULL;
	mCurrentGraphicsPipelineState = NULL;
	mCurrentComputeRootSignature = NULL;
	mCurrentComputePipelineState = NULL;
	mNumBarriersToFlush = 0;

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommandAllocator(commandType, IID_PPV_ARGS(&mCommandAllocator)));
	mCommandAllocator->SetName(L"Direct3DContext::mCommandAllocator");
	Direct3DUtils::ThrowIfHRESULTFailed(mCommandAllocator->Reset());

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

	Direct3DUtils::ThrowIfHRESULTFailed(mCommandAllocator->Reset());

	// Reset the command list
	Direct3DUtils::ThrowIfHRESULTFailed(mCommandList->Reset(mCommandAllocator, NULL));

	//flush current state, rebind descriptor heaps
	mCurrentGraphicsRootSignature = NULL;
	mCurrentComputeRootSignature = NULL;

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
		if (mCurrentDescriptorHeaps[i] != NULL)
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

void Direct3DContext::CopyDescriptors(uint32 numDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE destinationStart, D3D12_CPU_DESCRIPTOR_HANDLE sourceStart, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
	mDevice->CopyDescriptorsSimple(1, destinationStart, sourceStart, heapType);
}

void Direct3DContext::TransitionResource(GPUResource &resource, D3D12_RESOURCE_STATES newState, bool flushImmediate /* = false */)
{
	D3D12_RESOURCE_STATES oldState = resource.GetUsageState();

	if (mContextType == D3D12_COMMAND_LIST_TYPE_COMPUTE)
	{
		Application::Assert((oldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == oldState);
		Application::Assert((newState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == newState);
	}

	if (oldState != newState)
	{
		Application::Assert(mNumBarriersToFlush < BARRIER_LIMIT);
		D3D12_RESOURCE_BARRIER &barrierDesc = mResourceBarrierBuffer[mNumBarriersToFlush];
		mNumBarriersToFlush++;

		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = resource.GetResource();
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = oldState;
		barrierDesc.Transition.StateAfter = newState;

		// Check to see if we already started the transition
		if (newState == resource.GetTransitionState())
		{
			barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
			resource.SetTransitionState(D3D12_GPU_RESOURCE_STATE_UNKNOWN);
		}
		else
		{
			barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		}

		resource.SetUsageState(newState);
	}
	else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		InsertUAVBarrier(resource, flushImmediate);
	}

	if (flushImmediate || mNumBarriersToFlush == BARRIER_LIMIT)
	{
		FlushResourceBarriers();
	}
}

void Direct3DContext::InsertUAVBarrier(GPUResource &resource, bool flushImmediate /* = false */)
{
	Application::Assert(mNumBarriersToFlush < 16);
	D3D12_RESOURCE_BARRIER& barrierDesc = mResourceBarrierBuffer[mNumBarriersToFlush];
	mNumBarriersToFlush++;

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.UAV.pResource = resource.GetResource();

	if (flushImmediate)
	{
		FlushResourceBarriers();
	}
}

void Direct3DContext::InsertAliasBarrier(GPUResource &before, GPUResource &after, bool flushImmediate /* = false */)
{
	Application::Assert(mNumBarriersToFlush < 16);
	D3D12_RESOURCE_BARRIER& barrierDesc = mResourceBarrierBuffer[mNumBarriersToFlush];
	mNumBarriersToFlush++;

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.Aliasing.pResourceBefore = before.GetResource();
	barrierDesc.Aliasing.pResourceAfter = after.GetResource();

	if (flushImmediate)
	{
		FlushResourceBarriers();
	}
}


GraphicsContext::GraphicsContext(ID3D12Device *device)
	: Direct3DContext(device, D3D12_COMMAND_LIST_TYPE_DIRECT)
{

}

GraphicsContext::~GraphicsContext()
{

}

void GraphicsContext::SetRootSignature(ID3D12RootSignature *rootSignature)
{
	if (rootSignature == mCurrentGraphicsRootSignature)
	{
		return;
	}

	mCurrentGraphicsRootSignature = rootSignature;
	mCommandList->SetGraphicsRootSignature(mCurrentGraphicsRootSignature);
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

void GraphicsContext::SetPipelineState(ShaderPSO *pipeline)
{
	if (mCurrentGraphicsPipelineState == pipeline->GetPipelineState())
	{
		return;
	}

	mCurrentGraphicsPipelineState = pipeline->GetPipelineState();
	mCommandList->SetPipelineState(mCurrentGraphicsPipelineState);
}

void GraphicsContext::SetConstants(uint32 index, uint32 numConstants, const void *bufferData)
{
	mCommandList->SetGraphicsRoot32BitConstants(index, numConstants, bufferData, 0);
}

void GraphicsContext::SetRootConstantBuffer(uint32 index, ConstantBuffer *constantBuffer)
{
	mCommandList->SetGraphicsRootConstantBufferView(index, constantBuffer->GetGpuAddress());
}

void GraphicsContext::SetDescriptorTable(uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
	mCommandList->SetGraphicsRootDescriptorTable(index, handle);
}

void GraphicsContext::SetIndexBuffer(IndexBuffer *indexBuffer)
{
	if (!indexBuffer)
	{
		mCommandList->IASetIndexBuffer(NULL);
	}
	else
	{
		D3D12_INDEX_BUFFER_VIEW indexBufferView = indexBuffer->GetIndexBufferView();
		mCommandList->IASetIndexBuffer(&indexBufferView);
	}
}

void GraphicsContext::SetVertexBuffer(uint32 slot, VertexBuffer *vertexBuffer)
{
	if (!vertexBuffer)
	{
		mCommandList->IASetVertexBuffers(slot, 1, NULL);
	}
	else
	{
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = vertexBuffer->GetVertexBufferView();
		mCommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
	}
}

void GraphicsContext::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, float color[4])
{
	mCommandList->ClearRenderTargetView(target, color, 0, NULL);
}

void GraphicsContext::ClearDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, float depth, uint8 stencil)
{
	mCommandList->ClearDepthStencilView(target, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, NULL);
}

void GraphicsContext::DrawFullScreenTriangle()
{
	SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	SetVertexBuffer(0, NULL);
	SetIndexBuffer(NULL);
	Draw(3);
}

void GraphicsContext::Draw(uint32 vertexCount, uint32 vertexStartOffset /* = 0 */)
{
	DrawInstanced(vertexCount, 1, vertexStartOffset, 0);
}

void GraphicsContext::DrawIndexed(uint32 indexCount, uint32 startIndexLocation /* = 0 */, int32 baseVertexLocation /* = 0 */)
{
	DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
}

void GraphicsContext::DrawInstanced(uint32 vertexCountPerInstance, uint32 instanceCount, uint32 startVertexLocation /* = 0 */, uint32 startInstanceLocation /* = 0 */)
{
	FlushResourceBarriers();
	mCommandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void GraphicsContext::DrawIndexedInstanced(uint32 indexCountPerInstance, uint32 instanceCount, uint32 startIndexLocation, int32 baseVertexLocation, uint32 startInstanceLocation)
{
	FlushResourceBarriers();
	mCommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

ComputeContext::ComputeContext(ID3D12Device *device)
    : Direct3DContext(device, D3D12_COMMAND_LIST_TYPE_COMPUTE)
{

}

ComputeContext::~ComputeContext()
{

}

void ComputeContext::SetRootSignature(ID3D12RootSignature *rootSignature)
{
    if (rootSignature == mCurrentComputeRootSignature)
    {
        return;
    }

    mCurrentComputeRootSignature = rootSignature;
    mCommandList->SetComputeRootSignature(mCurrentComputeRootSignature);
}

void ComputeContext::SetPipelineState(ShaderPSO *pipeline)
{
    if (mCurrentComputePipelineState == pipeline->GetPipelineState())
    {
        return;
    }

    mCurrentComputePipelineState = pipeline->GetPipelineState();
    mCommandList->SetPipelineState(mCurrentComputePipelineState);
}

void ComputeContext::SetConstants(uint32 index, uint32 numConstants, const void *bufferData)
{
    mCommandList->SetComputeRoot32BitConstants(index, numConstants, bufferData, 0);
}

void ComputeContext::SetRootConstantBuffer(uint32 index, ConstantBuffer *constantBuffer)
{
    mCommandList->SetComputeRootConstantBufferView(index, constantBuffer->GetGpuAddress());
}

void ComputeContext::SetDescriptorTable(uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
    mCommandList->SetComputeRootDescriptorTable(index, handle);
}

//TDA: fill this out when we have a buffer type for it
/*
void ComputeContext::ClearUAV(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandleInCurrentHeap, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, )
{

    mCommandList->ClearUnorderedAccessViewUint(gpuHandleInCurrentHeap, cpuHandle, )

    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicDescriptorHeap.UploadDirect(Target.GetUAV());
    const UINT ClearColor[4] = {};
    m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
}

void ComputeContext::ClearUAV(ColorBuffer& Target)
{
    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicDescriptorHeap.UploadDirect(Target.GetUAV());
    CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

    //TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
    const float* ClearColor = Target.GetClearColor().GetPtr();
    m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
}*/

void ComputeContext::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
{
    FlushResourceBarriers();
    mCommandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void ComputeContext::Dispatch1D(uint32 threadCountX, uint32 groupSizeX)
{
    Dispatch(MathHelper::DivideByMultipleOf(threadCountX, groupSizeX), 1, 1);
}

void ComputeContext::Dispatch2D(uint32 threadCountX, uint32 threadCountY, uint32 groupSizeX, uint32 groupSizeY)
{
    Dispatch(MathHelper::DivideByMultipleOf(threadCountX, groupSizeX), MathHelper::DivideByMultipleOf(threadCountY, groupSizeY), 1);
}

void ComputeContext::Dispatch3D(uint32 threadCountX, uint32 threadCountY, uint32 threadCountZ, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ)
{
    Dispatch(MathHelper::DivideByMultipleOf(threadCountX, groupSizeX), MathHelper::DivideByMultipleOf(threadCountY, groupSizeY), MathHelper::DivideByMultipleOf(threadCountZ, groupSizeZ));
}

UploadContext::UploadContext(ID3D12Device *device)
	:Direct3DContext(device, D3D12_COMMAND_LIST_TYPE_COPY)
{
	D3D12_HEAP_PROPERTIES uploadHeapProperties;
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 0;
	uploadHeapProperties.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC uploadBufferDesc = {};
	uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadBufferDesc.Width = uint32(UPLOAD_BUFFER_SIZE);
	uploadBufferDesc.Height = 1;
	uploadBufferDesc.DepthOrArraySize = 1;
	uploadBufferDesc.MipLevels = 1;
	uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	uploadBufferDesc.SampleDesc.Count = 1;
	uploadBufferDesc.SampleDesc.Quality = 0;
	uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadBufferDesc.Alignment = 0;

	//A note for reference in case I ever forget this, Microsoft + Nvidia warn against using the generic read state for performance
	//It's better to give specific states and transition between them. That said, upload heaps are required to be in the generic read state.
	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUploadBuffer.BufferResource)));

	D3D12_RANGE readRange = {};
	Direct3DUtils::ThrowIfHRESULTFailed(mUploadBuffer.BufferResource->Map(0, &readRange, reinterpret_cast<void**>(&mUploadBuffer.BufferAddress)));
	mUploadBuffer.BufferStart = 0;
	mUploadBuffer.BufferUsed = 0;
	mUploadSubmissionStart = 0;
	mUploadSubmissionUsed = 0;
}

UploadContext::~UploadContext()
{
	D3D12_RANGE range = {};
	mUploadBuffer.BufferResource->Unmap(0, &range);

	if (mUploadBuffer.BufferResource)
	{
		mUploadBuffer.BufferResource->Release();
		mUploadBuffer.BufferResource = NULL;
	}
}

bool UploadContext::ClearSubmissionIfFinished(Direct3DUpload &submission, Direct3DQueueManager *queueManager)
{
    if (submission.IsUploading && queueManager->GetCopyQueue()->IsFenceComplete(submission.FenceValue))
    {
        mUploadSubmissionStart = (mUploadSubmissionStart + 1) % MAX_GPU_UPLOADS;
        mUploadSubmissionUsed -= 1;
        mUploadBuffer.BufferStart = (mUploadBuffer.BufferStart + submission.UploadPadding) % UPLOAD_BUFFER_SIZE;
        Application::Assert(submission.UploadLocation == mUploadBuffer.BufferStart);
        Application::Assert(mUploadBuffer.BufferStart + submission.UploadSize <= UPLOAD_BUFFER_SIZE);
        mUploadBuffer.BufferStart = (mUploadBuffer.BufferStart + submission.UploadSize) % UPLOAD_BUFFER_SIZE;
        mUploadBuffer.BufferUsed -= (submission.UploadSize + submission.UploadPadding);
        submission.Reset();

        if (mUploadBuffer.BufferUsed == 0)
        {
            mUploadBuffer.BufferStart = 0;
        }

        return true;
    }

    return false;
}

void UploadContext::ClearFinishedUploads(uint64 flushCount, Direct3DQueueManager *queueManager)
{
	const uint32 uploadStart = mUploadSubmissionStart;
	const uint32 numUsedUploads = mUploadSubmissionUsed;
	uint32 numFlushed = 0;

    //clear pending free uploads first before waiting on anything
	for (uint32 i = 0; i < numUsedUploads; ++i)
	{
		Direct3DUpload &submission = mUploads[(uploadStart + i) % MAX_GPU_UPLOADS];
        numFlushed += ClearSubmissionIfFinished(submission, queueManager) ? 1 : 0;
	}

    //if we still need some uploads freed, wait for as many as we need to
    if (numFlushed < flushCount)
    {
        for (uint32 i = 0; i < numUsedUploads; ++i)
        {
            Direct3DUpload &submission = mUploads[(uploadStart + i) % MAX_GPU_UPLOADS];
            if (submission.IsUploading && !queueManager->GetCopyQueue()->IsFenceComplete(submission.FenceValue))
            {
                queueManager->GetCopyQueue()->WaitForFence(submission.FenceValue);
            }

            numFlushed += ClearSubmissionIfFinished(submission, queueManager) ? 1 : 0;

            if (numFlushed == flushCount)
            {
                break;
            }
        }
    }
}

bool UploadContext::CreateNewUpload(uint64 size, uint32 &uploadIndex)
{
	Application::Assert(mUploadSubmissionUsed <= MAX_GPU_UPLOADS);
	if (mUploadSubmissionUsed == MAX_GPU_UPLOADS)
	{
		return false;
	}

	const uint32 newSubmissionId = (mUploadSubmissionStart + mUploadSubmissionUsed) % MAX_GPU_UPLOADS;
	Application::Assert(mUploads[newSubmissionId].UploadSize == 0);
	Application::Assert(mUploadBuffer.BufferUsed <= UPLOAD_BUFFER_SIZE);

	if (size > (UPLOAD_BUFFER_SIZE - mUploadBuffer.BufferUsed))
	{
		return false;
	}

	const uint64 start = mUploadBuffer.BufferStart;
	const uint64 end = mUploadBuffer.BufferStart + mUploadBuffer.BufferUsed;
	uint64 allocOffset = UINT64_MAX;
	uint64 padding = 0;

	if (end < UPLOAD_BUFFER_SIZE)
	{
		const uint64 endAmount = UPLOAD_BUFFER_SIZE - end;
		if (endAmount >= size)
		{
			allocOffset = end;
		}
		else if (start >= size)
		{
			// Wrap around to the beginning
			allocOffset = 0;
			mUploadBuffer.BufferUsed += endAmount;
			padding = endAmount;
		}
	}
	else
	{
		const uint64 wrappedEnd = end % UPLOAD_BUFFER_SIZE;
		Application::Assert(start > wrappedEnd);
		if ((start - wrappedEnd) >= size)
		{
			allocOffset = wrappedEnd;
		}
	}

	if (allocOffset == UINT64_MAX)
	{
		return false;
	}

	mUploadSubmissionUsed += 1;
	mUploadBuffer.BufferUsed += size;

	mUploads[newSubmissionId].UploadLocation = allocOffset;
	mUploads[newSubmissionId].UploadSize = size;
	mUploads[newSubmissionId].UploadPadding = padding;
	uploadIndex = newSubmissionId;

	return true;
}

Direct3DUploadInfo UploadContext::BeginUpload(uint64 size, Direct3DQueueManager *queueManager)
{
	size = Application::Align(size, UPLOAD_BUFFER_ALIGNMENT);
	Application::Assert(size <= UPLOAD_BUFFER_SIZE && size > 0);

	uint32 uploadIndex = 0;

	ClearFinishedUploads(0, queueManager);
	while (!CreateNewUpload(size, uploadIndex))
	{
		ClearFinishedUploads(1, queueManager);
	}

	const uint32 submissionIdx = (mUploadSubmissionStart + (mUploadSubmissionUsed - 1)) % MAX_GPU_UPLOADS;
	Direct3DUpload &submission = mUploads[submissionIdx];

	Direct3DUploadInfo uploadInfo;
	uploadInfo.Resource = mUploadBuffer.BufferResource;
	uploadInfo.CPUAddress = mUploadBuffer.BufferAddress + submission.UploadLocation;
	uploadInfo.UploadAddressOffset = submission.UploadLocation;
	uploadInfo.UploadID = uploadIndex;

	return uploadInfo;
}

void UploadContext::CopyTextureRegion(D3D12_TEXTURE_COPY_LOCATION *destination, D3D12_TEXTURE_COPY_LOCATION *source)
{
	mCommandList->CopyTextureRegion(destination, 0, 0, 0, source, NULL);
}

void UploadContext::CopyResourceRegion(ID3D12Resource *destination, uint64 destOffset, ID3D12Resource *source, uint64 sourceOffset, uint64 numBytes)
{
	mCommandList->CopyBufferRegion(destination, destOffset, source, sourceOffset, numBytes);
}

uint64 UploadContext::FlushUpload(Direct3DUploadInfo& uploadInfo, Direct3DQueueManager *queueManager, bool forceWait) //TDA not actually using forcewait yet
{
	uint64 uploadFence = Flush(queueManager, true);
	mUploads[uploadInfo.UploadID].FenceValue = uploadFence;
	mUploads[uploadInfo.UploadID].IsUploading = true;

	return uploadFence;
}