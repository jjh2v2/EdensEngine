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

GPUBuffer *Direct3DContextManager::CreateGPUBuffer(ID3D12Device* device, uint32 elementCount, uint32 elementSize,
	const void* initData, size_t numBytes, bool useOffset /* = false */, size_t offset /* = 0 */)
{
	GPUBuffer *newBuffer = new GPUBuffer();

	newBuffer->SetBufferInfo(elementCount * elementSize, elementCount, elementSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	newBuffer->SetUsageState(D3D12_RESOURCE_STATE_COMMON);

	Application::Assert(newBuffer->GetBufferSize() > 0);

	D3D12_RESOURCE_DESC sourceBufferDesc;
	sourceBufferDesc.Alignment = 0;
	sourceBufferDesc.DepthOrArraySize = 1;
	sourceBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	sourceBufferDesc.Flags = newBuffer->GetResourceFlags();
	sourceBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	sourceBufferDesc.Height = 1;
	sourceBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	sourceBufferDesc.MipLevels = 1;
	sourceBufferDesc.SampleDesc.Count = 1;
	sourceBufferDesc.SampleDesc.Quality = 0;
	sourceBufferDesc.Width = (uint64)newBuffer->GetBufferSize();

	D3D12_HEAP_PROPERTIES sourceHeapProperties;
	sourceHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	sourceHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	sourceHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	sourceHeapProperties.CreationNodeMask = 1;
	sourceHeapProperties.VisibleNodeMask = 1;

	ID3D12Resource *newResource = NULL;

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommittedResource(&sourceHeapProperties, D3D12_HEAP_FLAG_NONE, &sourceBufferDesc, 
		newBuffer->GetUsageState(), NULL, IID_PPV_ARGS(&newResource)));

	newBuffer->SetResource(newResource);
	newBuffer->SetGPUAddress(newResource->GetGPUVirtualAddress());

	ID3D12Resource* uploadBuffer;

	D3D12_HEAP_PROPERTIES uploadHeapProperties;
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 1;
	uploadHeapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC uploadResourceDesc;
	uploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadResourceDesc.Alignment = 0;
	uploadResourceDesc.Width = numBytes;
	uploadResourceDesc.Height = 1;
	uploadResourceDesc.DepthOrArraySize = 1;
	uploadResourceDesc.MipLevels = 1;
	uploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadResourceDesc.SampleDesc.Count = 1;
	uploadResourceDesc.SampleDesc.Quality = 0;
	uploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&uploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer)));

	void* mapDestination;
	uploadBuffer->Map(0, NULL, &mapDestination);
	memcpy(mapDestination, initData, numBytes);
	uploadBuffer->Unmap(0, NULL);

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	mGraphicsContext->TransitionResource((*newBuffer), D3D12_RESOURCE_STATE_COPY_DEST, true);
	if (useOffset)
	{
		mGraphicsContext->GetCommandList()->CopyBufferRegion(newResource, offset, uploadBuffer, 0, numBytes);
	}
	else
	{
		mGraphicsContext->GetCommandList()->CopyResource(newResource, uploadBuffer);
	}

	mGraphicsContext->TransitionResource((*newBuffer), D3D12_RESOURCE_STATE_GENERIC_READ, true);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	mGraphicsContext->Flush(mQueueManager, true);

	uploadBuffer->Release();

	newResource->SetName(L"GPUBuffer::mResource");
	return newBuffer;
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