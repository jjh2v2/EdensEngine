#include "Direct3DContextManager.h"

Direct3DContextManager::Direct3DContextManager(ID3D12Device* device)
{
	mQueueManager = new Direct3DQueueManager(device);
	mGraphicsContext = new GraphicsContext(device);
}

Direct3DContextManager::~Direct3DContextManager()
{
	delete mGraphicsContext;
	delete mQueueManager;
}

void Direct3DContextManager::InitializeBuffer(ID3D12Device* device, GPUResource *resource, const void* initData, size_t numBytes, bool useOffset /* = false */, size_t offset /* = 0 */)
{
	ID3D12Resource* uploadBuffer;

	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = numBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer)));

	void* mapDestination;
	uploadBuffer->Map(0, NULL, &mapDestination);
	memcpy(mapDestination, initData, numBytes);
	uploadBuffer->Unmap(0, NULL);

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	mGraphicsContext->TransitionResource((*resource), D3D12_RESOURCE_STATE_COPY_DEST, true);
	if (useOffset)
	{
		mGraphicsContext->GetCommandList()->CopyBufferRegion(resource->GetResource(), offset, uploadBuffer, 0, numBytes);
	}
	else
	{
		mGraphicsContext->GetCommandList()->CopyResource(resource->GetResource(), uploadBuffer);
	}

	mGraphicsContext->TransitionResource((*resource), D3D12_RESOURCE_STATE_GENERIC_READ, true);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	mGraphicsContext->Flush(mQueueManager, true);

	uploadBuffer->Release();
}

/*void CommandContext::InitializeTexture(GPUResource& destination, UINT numSubresources, D3D12_SUBRESOURCE_DATA subresourceData[])
{
	ID3D12Resource* UploadBuffer;

	UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest.GetResource(), 0, NumSubresources);

	CommandContext& InitContext = CommandContext::Begin();

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC BufferDesc;
	BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	BufferDesc.Alignment = 0;
	BufferDesc.Width = uploadBufferSize;
	BufferDesc.Height = 1;
	BufferDesc.DepthOrArraySize = 1;
	BufferDesc.MipLevels = 1;
	BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	BufferDesc.SampleDesc.Count = 1;
	BufferDesc.SampleDesc.Quality = 0;
	BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ASSERT_SUCCEEDED(Graphics::g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
		&BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, MY_IID_PPV_ARGS(&UploadBuffer)));

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	UpdateSubresources(InitContext.m_CommandList, Dest.GetResource(), UploadBuffer, 0, 0, NumSubresources, SubData);
	InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	InitContext.Finish(true);

	UploadBuffer->Release();
}*/