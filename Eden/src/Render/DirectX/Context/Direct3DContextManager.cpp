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

	mUploadContext->FlushUpload(uploadInfo, mQueueManager);

	VertexBuffer *vertexBuffer = new VertexBuffer(vertexBufferResource, D3D12_RESOURCE_STATE_COPY_DEST, vertexStride, bufferSize);

	mGraphicsContext->TransitionResource((*vertexBuffer), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, true);

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

	mUploadContext->FlushUpload(uploadInfo, mQueueManager);

	IndexBuffer *indexBuffer = new IndexBuffer(indexBufferResource, D3D12_RESOURCE_STATE_COPY_DEST, bufferSize);

	mGraphicsContext->TransitionResource((*indexBuffer), D3D12_RESOURCE_STATE_INDEX_BUFFER, true);

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

RenderTarget *Direct3DContextManager::CreateRenderTarget(uint32 width, uint32 height, DXGI_FORMAT format, bool hasUAV, uint16 arraySize, uint32 sampleCount, uint32 quality)
{
	D3D12_RESOURCE_DESC targetDesc = {};
	targetDesc.Width = width;
	targetDesc.Height = height;
	targetDesc.Format = format;
	targetDesc.DepthOrArraySize = arraySize;
	targetDesc.MipLevels = 1;
	targetDesc.Alignment = 0;
	targetDesc.SampleDesc.Count = sampleCount;
	targetDesc.SampleDesc.Quality = sampleCount > 1 ? quality : 0;
	targetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	targetDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	targetDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (hasUAV)
	{
		targetDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	//Make sure the target is cleared with ClearRenderTarget to the same value as the below (currently 0,0,0,0).
	//Documentation: The clear values do not match those passed to resource creation. The clear operation is typically slower as a result; 
	//but will still clear to the desired value.
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;

	D3D12_HEAP_PROPERTIES defaultHeapProperties;
	defaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	defaultHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	defaultHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	defaultHeapProperties.CreationNodeMask = 0;
	defaultHeapProperties.VisibleNodeMask = 0;

	ID3D12Resource *renderTargetResource = NULL;
	Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &targetDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&renderTargetResource)));

	DescriptorHeapHandle srvHandle = mHeapManager->GetNewSRVDescriptorHeapHandle();
	DescriptorHeapHandle rtvHandle = mHeapManager->GetNewRTVDescriptorHeapHandle();

	mDevice->CreateShaderResourceView(renderTargetResource, NULL, srvHandle.GetCPUHandle());
	mDevice->CreateRenderTargetView(renderTargetResource, NULL, rtvHandle.GetCPUHandle());

	RenderTarget::UAVHandle uavHandle;
	uavHandle.HasUAV = hasUAV;

	if (hasUAV)
	{
		uavHandle.Handle = mHeapManager->GetNewSRVDescriptorHeapHandle();
		mDevice->CreateUnorderedAccessView(renderTargetResource, NULL, NULL, uavHandle.Handle.GetCPUHandle());
	}

	DynamicArray<DescriptorHeapHandle> rtvArrayHeapHandles;

	if (arraySize > 1)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Format = format;
		if (sampleCount > 1)
		{
			rtvDesc.Texture2DMSArray.ArraySize = 1;
		}
		else
		{
			rtvDesc.Texture2DArray.ArraySize = 1;
		}

		for (uint32 i = 0; i < arraySize; i++)
		{
			if (sampleCount > 1)
			{
				rtvDesc.Texture2DMSArray.FirstArraySlice = (uint32)i;
			}
			else
			{
				rtvDesc.Texture2DArray.FirstArraySlice = (uint32)i;
			}

			DescriptorHeapHandle rtvArrayHandle = mHeapManager->GetNewRTVDescriptorHeapHandle();
			mDevice->CreateRenderTargetView(renderTargetResource, &rtvDesc, rtvArrayHandle.GetCPUHandle());
			rtvArrayHeapHandles.Add(rtvArrayHandle);
		}
	}

	RenderTarget *renderTarget = new RenderTarget(renderTargetResource, D3D12_RESOURCE_STATE_RENDER_TARGET, targetDesc, rtvHandle, rtvArrayHeapHandles,
		srvHandle, uavHandle);

	return renderTarget;
}

/*
These are valid formats for a depth-stencil view:
DXGI_FORMAT_D16_UNORM
DXGI_FORMAT_D24_UNORM_S8_UINT
DXGI_FORMAT_D32_FLOAT
DXGI_FORMAT_D32_FLOAT_S8X24_UINT
DXGI_FORMAT_UNKNOWN
A depth-stencil view can't use a typeless format. If the format chosen is DXGI_FORMAT_UNKNOWN, the format of the parent resource is used.
https://msdn.microsoft.com/en-us/library/windows/desktop/dn770357(v=vs.85).aspx
*/
DepthStencilTarget *Direct3DContextManager::CreateDepthStencilTarget(uint32 width, uint32 height, DXGI_FORMAT format, uint16 arraySize, uint32 sampleCount, uint32 quality)
{
	DXGI_FORMAT depthStencilResourceFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT shaderResourceViewFormat = DXGI_FORMAT_UNKNOWN;
	bool hasStencil = (format == DXGI_FORMAT_D24_UNORM_S8_UINT) || (format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT);

	switch (format)
	{
	case DXGI_FORMAT_D16_UNORM:
		depthStencilResourceFormat = DXGI_FORMAT_R16_TYPELESS;
		shaderResourceViewFormat = DXGI_FORMAT_R16_UNORM;
		break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		depthStencilResourceFormat = DXGI_FORMAT_R24G8_TYPELESS;
		shaderResourceViewFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT:
		depthStencilResourceFormat = DXGI_FORMAT_R32_TYPELESS;
		shaderResourceViewFormat = DXGI_FORMAT_R32_FLOAT;
		break;
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		depthStencilResourceFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
		shaderResourceViewFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		break;
	default:
		Direct3DUtils::ThrowRuntimeError("Not using a valid depth-stencil format, see comment above Direct3DContextManager::CreateDepthStencilTarget");
		break;
	}

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.Format = depthStencilResourceFormat;
	textureDesc.DepthOrArraySize = arraySize;
	textureDesc.MipLevels = 1;
	textureDesc.Alignment = 0;
	textureDesc.SampleDesc.Count = sampleCount;
	textureDesc.SampleDesc.Quality = sampleCount > 1 ? quality : 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.DepthStencil.Depth = 0.0f;
	clearValue.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES defaultHeapProperties;
	defaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	defaultHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	defaultHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	defaultHeapProperties.CreationNodeMask = 0;
	defaultHeapProperties.VisibleNodeMask = 0;

	ID3D12Resource *depthStencilResource = NULL;
	Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthStencilResource)));

	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = shaderResourceViewFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = format;

	if (sampleCount == 1)
	{
		if (arraySize > 1)
		{
			srvDesc.Texture2DArray.ArraySize = (uint32)arraySize;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.PlaneSlice = 0;
			srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.ArraySize = (uint32)arraySize;
			dsvDesc.Texture2DArray.FirstArraySlice = 0;
			dsvDesc.Texture2DArray.MipSlice = 0;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		}
		else
		{
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		}
		
	}
	else
	{
		if (arraySize > 1)
		{
			srvDesc.Texture2DMSArray.FirstArraySlice = 0;
			srvDesc.Texture2DMSArray.ArraySize = (uint32)arraySize;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
			dsvDesc.Texture2DMSArray.ArraySize = (uint32)arraySize;
			dsvDesc.Texture2DMSArray.FirstArraySlice = 0;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		}
		else
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		}
		
	}

	DescriptorHeapHandle srvHandle = mHeapManager->GetNewSRVDescriptorHeapHandle();
	mDevice->CreateShaderResourceView(depthStencilResource, &srvDesc, srvHandle.GetCPUHandle());

	DescriptorHeapHandle dsvHandle = mHeapManager->GetNewDSVDescriptorHeapHandle();
	mDevice->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvHandle.GetCPUHandle());

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	dsvDesc.Flags |= hasStencil ? D3D12_DSV_FLAG_READ_ONLY_STENCIL : D3D12_DSV_FLAG_NONE;
	DescriptorHeapHandle readOnlyDSVHandle = mHeapManager->GetNewDSVDescriptorHeapHandle();
	mDevice->CreateDepthStencilView(depthStencilResource, &dsvDesc, readOnlyDSVHandle.GetCPUHandle());

	DynamicArray<DescriptorHeapHandle> dsvArrayHeapHandles;
	DynamicArray<DescriptorHeapHandle> readOnlyDSVArrayHeapHandles;

	if (arraySize > 1)
	{
		if (sampleCount > 1)
		{
			dsvDesc.Texture2DMSArray.ArraySize = 1;
		}
		else
		{
			dsvDesc.Texture2DArray.ArraySize = 1;
		}

		for (uint32 i = 0; i < arraySize; i++)
		{
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

			if (sampleCount > 1)
			{
				dsvDesc.Texture2DMSArray.FirstArraySlice = i;
			}
			else
			{
				dsvDesc.Texture2DArray.FirstArraySlice = i;
			}

			DescriptorHeapHandle dsvArrayHandle = mHeapManager->GetNewRTVDescriptorHeapHandle();
			mDevice->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvArrayHandle.GetCPUHandle());
			dsvArrayHeapHandles.Add(dsvArrayHandle);

			dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
			dsvDesc.Flags |= hasStencil ? D3D12_DSV_FLAG_READ_ONLY_STENCIL : D3D12_DSV_FLAG_NONE;
			DescriptorHeapHandle readOnlyDSVArrayHandle = mHeapManager->GetNewRTVDescriptorHeapHandle();
			mDevice->CreateDepthStencilView(depthStencilResource, &dsvDesc, readOnlyDSVArrayHandle.GetCPUHandle());
			readOnlyDSVArrayHeapHandles.Add(readOnlyDSVArrayHandle);
		}
	}

	DepthStencilTarget *newDepthStencilTarget = new DepthStencilTarget(depthStencilResource, D3D12_RESOURCE_STATE_DEPTH_WRITE, textureDesc, dsvHandle, readOnlyDSVHandle,
		dsvArrayHeapHandles, readOnlyDSVArrayHeapHandles, srvHandle);

	return newDepthStencilTarget;
}

void Direct3DContextManager::FreeRenderTarget(RenderTarget *renderTarget)
{
	mHeapManager->FreeRTVDescriptorHeapHandle(renderTarget->GetRenderTargetViewHandle());
	mHeapManager->FreeSRVDescriptorHeapHandle(renderTarget->GetShaderResourceViewHandle());
	RenderTarget::UAVHandle uavHandle = renderTarget->GetUnorderedAccessViewHandle();

	if (uavHandle.HasUAV)
	{
		mHeapManager->FreeSRVDescriptorHeapHandle(uavHandle.Handle);
	}

	uint16 targetArraySize = renderTarget->GetArraySize() - 1;
	for (uint16 rtvArrayIndex = 0; rtvArrayIndex < targetArraySize; rtvArrayIndex++)
	{
		mHeapManager->FreeRTVDescriptorHeapHandle(renderTarget->GetRenderTargetViewHandle(rtvArrayIndex));
	}

	delete renderTarget;
}

void Direct3DContextManager::FreeDepthStencilTarget(DepthStencilTarget *depthStencilTarget)
{
	mHeapManager->FreeDSVDescriptorHeapHandle(depthStencilTarget->GetDepthStencilViewHandle());
	mHeapManager->FreeDSVDescriptorHeapHandle(depthStencilTarget->GetReadOnlyDepthStencilViewHandle());
	mHeapManager->FreeSRVDescriptorHeapHandle(depthStencilTarget->GetShaderResourceViewHandle());

	uint16 targetArraySize = depthStencilTarget->GetArraySize() - 1;
	for (uint16 dsvArrayIndex = 0; dsvArrayIndex < targetArraySize; dsvArrayIndex++)
	{
		mHeapManager->FreeDSVDescriptorHeapHandle(depthStencilTarget->GetDepthStencilViewHandle(dsvArrayIndex));
		mHeapManager->FreeDSVDescriptorHeapHandle(depthStencilTarget->GetReadOnlyDepthStencilViewHandle(dsvArrayIndex));
	}

	delete depthStencilTarget;
}

void Direct3DContextManager::FreeConstantBuffer(ConstantBuffer *constantBuffer)
{
	mHeapManager->FreeSRVDescriptorHeapHandle(constantBuffer->GetConstantBufferViewHandle());
	delete constantBuffer;
}