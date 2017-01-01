#include "Direct3DContextManager.h"

Direct3DContextManager::Direct3DContextManager(ID3D12Device* device)
{
	mDevice = device;
	mHeapManager = new Direct3DHeapManager(mDevice);
	mQueueManager = new Direct3DQueueManager(mDevice);
	mUploadContext = new UploadContext(mDevice);
	mGraphicsContext = new GraphicsContext(mDevice);
}

Direct3DContextManager::~Direct3DContextManager()
{
	delete mUploadContext;
	delete mGraphicsContext;
	delete mQueueManager;
	delete mHeapManager;
}

VertexBuffer *Direct3DContextManager::CreateVertexBuffer(void* vertexData, uint32 vertexStride, uint32 bufferSize)
{
	ID3D12Resource *vertexBufferResource = NULL;

	D3D12_RESOURCE_DESC vertexBufferDesc;
	vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexBufferDesc.Alignment = 0;
	vertexBufferDesc.Width = bufferSize;
	vertexBufferDesc.Height = 1;
	vertexBufferDesc.DepthOrArraySize = 1;
	vertexBufferDesc.MipLevels = 1;
	vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	vertexBufferDesc.SampleDesc.Count = 1;
	vertexBufferDesc.SampleDesc.Quality = 0;
	vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	vertexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES defaultHeapProperties;
	defaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	defaultHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	defaultHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	defaultHeapProperties.CreationNodeMask = 0;
	defaultHeapProperties.VisibleNodeMask = 0;

	Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&vertexBufferResource)));

	Direct3DUploadInfo uploadInfo = mUploadContext->BeginUpload(bufferSize, mQueueManager);
	uint8* uploadMem = reinterpret_cast<uint8*>(uploadInfo.CPUAddress);

	memcpy(uploadMem, vertexData, bufferSize);
	mUploadContext->CopyResourceRegion(vertexBufferResource, 0, uploadInfo.Resource, uploadInfo.ResourceOffset, bufferSize);

	mUploadContext->EndUpload(uploadInfo, mQueueManager);

	VertexBuffer *vertexBuffer = new VertexBuffer(vertexBufferResource, D3D12_RESOURCE_STATE_COPY_DEST, vertexStride, bufferSize);

	mGraphicsContext->TransitionResource((*vertexBuffer), D3D12_RESOURCE_STATE_GENERIC_READ, true);

	return vertexBuffer;
}

IndexBuffer *Direct3DContextManager::CreateIndexBuffer(void* indexData, uint32 bufferSize)
{
	ID3D12Resource *indexBufferResource = NULL;

	D3D12_RESOURCE_DESC indexBufferDesc;
	indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	indexBufferDesc.Alignment = 0;
	indexBufferDesc.Width = bufferSize;
	indexBufferDesc.Height = 1;
	indexBufferDesc.DepthOrArraySize = 1;
	indexBufferDesc.MipLevels = 1;
	indexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	indexBufferDesc.SampleDesc.Count = 1;
	indexBufferDesc.SampleDesc.Quality = 0;
	indexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	indexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES defaultHeapProperties;
	defaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	defaultHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	defaultHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	defaultHeapProperties.CreationNodeMask = 0;
	defaultHeapProperties.VisibleNodeMask = 0;

	Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&indexBufferResource)));

	Direct3DUploadInfo uploadInfo = mUploadContext->BeginUpload(bufferSize, mQueueManager);
	uint8* uploadMem = reinterpret_cast<uint8*>(uploadInfo.CPUAddress);

	memcpy(uploadMem, indexData, bufferSize);
	mUploadContext->CopyResourceRegion(indexBufferResource, 0, uploadInfo.Resource, uploadInfo.ResourceOffset, bufferSize);

	mUploadContext->EndUpload(uploadInfo, mQueueManager);

	IndexBuffer *indexBuffer = new IndexBuffer(indexBufferResource, D3D12_RESOURCE_STATE_COPY_DEST, bufferSize);

	mGraphicsContext->TransitionResource((*indexBuffer), D3D12_RESOURCE_STATE_GENERIC_READ, true);

	return indexBuffer;
}

//note, usually people create constant buffers with size * frame count - so you can update while another is being used. (not sure if this works or if you need separate buffers)
ConstantBuffer *Direct3DContextManager::CreateConstantBuffer(uint32 bufferSize)
{
	ID3D12Resource *constantBufferResource = NULL;
	uint32 alignedSize = MathHelper::AlignU32(bufferSize, 256); // Constant buffer size is required to be 256-byte aligned.

	D3D12_RESOURCE_DESC constantBufferDesc;
	constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	constantBufferDesc.Alignment = 0;
	constantBufferDesc.Width = alignedSize;
	constantBufferDesc.Height = 1;
	constantBufferDesc.DepthOrArraySize = 1;
	constantBufferDesc.MipLevels = 1;
	constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	constantBufferDesc.SampleDesc.Count = 1;
	constantBufferDesc.SampleDesc.Quality = 0;
	constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	constantBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES uploadHeapProperties;
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 0;
	uploadHeapProperties.VisibleNodeMask = 0;

	Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&constantBufferResource)));

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
	constantBufferViewDesc.BufferLocation = constantBufferResource->GetGPUVirtualAddress();
	constantBufferViewDesc.SizeInBytes = alignedSize;

	DescriptorHeapHandle constantBufferHeapHandle = mHeapManager->GetNewSRVDescriptorHeapHandle();
	mDevice->CreateConstantBufferView(&constantBufferViewDesc, constantBufferHeapHandle.GetCPUHandle());

	ConstantBuffer *constantBuffer = new ConstantBuffer(constantBufferResource, D3D12_RESOURCE_STATE_GENERIC_READ, constantBufferViewDesc, constantBufferHeapHandle);
	return constantBuffer;
}