#include "Direct3DContextManager.h"

Direct3DContextManager::Direct3DContextManager(ID3D12Device* device, bool isRayTracingSupported)
{
	mDevice = device;
	mHeapManager = new Direct3DHeapManager(mDevice);
	mQueueManager = new Direct3DQueueManager(mDevice);
	mUploadContext = new UploadContext(mDevice);
    mComputeContext = new ComputeContext(mDevice);
	mGraphicsContext = new GraphicsContext(mDevice);

    mRayTraceContext = NULL;

    if (isRayTracingSupported)
    {
        mRayTraceContext = new RayTraceContext(mDevice);
    }

    memset(mBufferTracking, 0, ContextTrackingType_Max * sizeof(uint32));
}

Direct3DContextManager::~Direct3DContextManager()
{
    if (mRayTraceContext)
    {
        delete mRayTraceContext;
    }

	delete mUploadContext;
    delete mComputeContext;
	delete mGraphicsContext;
	delete mQueueManager;
	delete mHeapManager;

    for (uint32 i = 0; i < ContextTrackingType_Max; i++)
    {
        Application::Assert(mBufferTracking[i] == 0);
    }
}

void Direct3DContextManager::FinishContextsAndWaitForIdle()
{
    if (mRayTraceContext)
    {
        mRayTraceContext->Flush(mQueueManager, true, true);
    }
    
    mComputeContext->Flush(mQueueManager, true, true);
    mGraphicsContext->Flush(mQueueManager, true, true);
    mUploadContext->Flush(mQueueManager, true, true);

    if (mRayTraceContext)
    {
        mRayTraceContext->WaitForCommandIdle(mQueueManager);
    }
    
    mUploadContext->WaitForCommandIdle(mQueueManager);
    mComputeContext->WaitForCommandIdle(mQueueManager);
    mGraphicsContext->WaitForCommandIdle(mQueueManager);
}

Direct3DContextManager::VertexBufferBackgroundUpload::VertexBufferBackgroundUpload(Direct3DContextManager *contextManager, VertexBuffer *vertexBuffer, void *vertexData)
{
    mContextManager = contextManager;
    mVertexBuffer = vertexBuffer;
    mVertexData = vertexData;
}

void Direct3DContextManager::VertexBufferBackgroundUpload::ProcessUpload(UploadContext *uploadContext)
{
    uint32 bufferSize = mVertexBuffer->GetVertexBufferView().SizeInBytes;
    Direct3DUploadInfo uploadInfo = uploadContext->BeginUpload(bufferSize, mContextManager->GetQueueManager());
    uint8* uploadMem = reinterpret_cast<uint8*>(uploadInfo.CPUAddress);

    memcpy(uploadMem, mVertexData, bufferSize);
    uploadContext->CopyResourceRegion(mVertexBuffer->GetResource(), 0, uploadInfo.Resource, uploadInfo.UploadAddressOffset, bufferSize);

    mUploadFence = uploadContext->FlushUpload(uploadInfo, mContextManager->GetQueueManager());
}

void Direct3DContextManager::VertexBufferBackgroundUpload::OnUploadFinished()
{
    mContextManager->GetGraphicsContext()->TransitionResourceDeferred(mVertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, true);
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

	VertexBuffer *vertexBuffer = new VertexBuffer(vertexBufferResource, D3D12_RESOURCE_STATE_COPY_DEST, vertexStride, bufferSize);

    VertexBufferBackgroundUpload *vertexBufferUpload = new VertexBufferBackgroundUpload(this, vertexBuffer, vertexData);
    mUploadContext->AddBackgroundUpload(vertexBufferUpload);

	return vertexBuffer;
}

Direct3DContextManager::IndexBufferBackgroundUpload::IndexBufferBackgroundUpload(Direct3DContextManager *contextManager, IndexBuffer *indexBuffer, void *indexData)
{
    mContextManager = contextManager;
    mIndexBuffer = indexBuffer;
    mIndexData = indexData;
}

void Direct3DContextManager::IndexBufferBackgroundUpload::ProcessUpload(UploadContext *uploadContext)
{
    uint32 bufferSize = mIndexBuffer->GetIndexBufferView().SizeInBytes;
    Direct3DUploadInfo uploadInfo = uploadContext->BeginUpload(bufferSize, mContextManager->GetQueueManager());
    uint8* uploadMem = reinterpret_cast<uint8*>(uploadInfo.CPUAddress);

    memcpy(uploadMem, mIndexData, bufferSize);
    uploadContext->CopyResourceRegion(mIndexBuffer->GetResource(), 0, uploadInfo.Resource, uploadInfo.UploadAddressOffset, bufferSize);

    mUploadFence = uploadContext->FlushUpload(uploadInfo, mContextManager->GetQueueManager());
}

void Direct3DContextManager::IndexBufferBackgroundUpload::OnUploadFinished()
{
    mContextManager->GetGraphicsContext()->TransitionResourceDeferred(mIndexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER, true);
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

	IndexBuffer *indexBuffer = new IndexBuffer(indexBufferResource, D3D12_RESOURCE_STATE_COPY_DEST, bufferSize);

    IndexBufferBackgroundUpload *indexBufferUpload = new IndexBufferBackgroundUpload(this, indexBuffer, indexData);
    mUploadContext->AddBackgroundUpload(indexBufferUpload);

	return indexBuffer;
}

//note, possibly create constant buffers with size * frame count - so you can update while another is being used. (not sure if this works or if you need separate buffers)
ConstantBuffer *Direct3DContextManager::CreateConstantBuffer(uint32 bufferSize)
{
	ID3D12Resource *constantBufferResource = NULL;
	uint32 alignedSize = MathHelper::AlignU32(bufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT); // Constant buffer size is required to be 256-byte aligned.

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
    constantBuffer->SetIsReady(true);

    mBufferTracking[ContextTrackingType_ConstantBuffer]++;

    return constantBuffer;
}

StructuredBuffer *Direct3DContextManager::CreateStructuredBuffer(uint32 elementSize, uint32 numElements, StructuredBufferAccess accessType, bool isRaw)
{
    Application::Assert(elementSize % 16 == 0); //ensure elements are 16 byte aligned so that they don't span cache lines

    D3D12_RESOURCE_DESC structuredBufferDesc;
    structuredBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    structuredBufferDesc.Alignment = 0;
    structuredBufferDesc.Width = elementSize * numElements;
    structuredBufferDesc.Height = 1;
    structuredBufferDesc.DepthOrArraySize = 1;
    structuredBufferDesc.MipLevels = 1;
    structuredBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    structuredBufferDesc.SampleDesc.Count = 1;
    structuredBufferDesc.SampleDesc.Quality = 0;
    structuredBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    structuredBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    
    ID3D12Resource *structuredBufferResource = NULL;
    ID3D12Resource *structuredBufferUploadResource = NULL;
    D3D12_RESOURCE_STATES bufferResourceState = D3D12_RESOURCE_STATE_COMMON;

    D3D12_HEAP_PROPERTIES defaultHeapProperties;
    defaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    defaultHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    defaultHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    defaultHeapProperties.CreationNodeMask = 0;
    defaultHeapProperties.VisibleNodeMask = 0;

    D3D12_HEAP_PROPERTIES uploadHeapProperties;
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    uploadHeapProperties.CreationNodeMask = 0;
    uploadHeapProperties.VisibleNodeMask = 0;

    //Resources for this end up being pretty different based on how it's going to be used:
    //GPU R/W is standard, just creates a single resource that is never accessed by the CPU
    //CPU W GPU R sets up the resource on the upload heap like a constant buffer.
    //---The assumption being that frequent CPU writes means this is the most efficient method. Needs perf testing to back it up though.
    //CPU W GPU R/W creates two resources, one on the default heap, and one on the upload heap. 
    //---The CPU writes to the upload resource, and then the managing system needs to manually schedule a GPU copy to the primary resource
    switch (accessType)
    {
    case GPU_READ_WRITE:
        bufferResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &structuredBufferDesc,
            bufferResourceState, NULL, IID_PPV_ARGS(&structuredBufferResource)));

        break;
    case CPU_WRITE_GPU_READ:
        bufferResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;

        Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &structuredBufferDesc,
            bufferResourceState, NULL, IID_PPV_ARGS(&structuredBufferResource)));
        break;
    case CPU_WRITE_GPU_READ_WRITE:
        bufferResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &structuredBufferDesc,
            bufferResourceState, NULL, IID_PPV_ARGS(&structuredBufferResource)));

        Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &structuredBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&structuredBufferUploadResource)));
        break;
    default:
        Application::Assert(false);
        break;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = numElements;
    srvDesc.Buffer.StructureByteStride = elementSize;
    srvDesc.Buffer.Flags = isRaw ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
    
    DescriptorHeapHandle srvHandle = mHeapManager->GetNewSRVDescriptorHeapHandle();
    mDevice->CreateShaderResourceView(structuredBufferResource, &srvDesc, srvHandle.GetCPUHandle());

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = numElements;
    uavDesc.Buffer.StructureByteStride = elementSize;
    uavDesc.Buffer.Flags = isRaw ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

    DescriptorHeapHandle uavHandle = mHeapManager->GetNewSRVDescriptorHeapHandle();
    mDevice->CreateUnorderedAccessView(structuredBufferResource, NULL, &uavDesc, uavHandle.GetCPUHandle());

    StructuredBuffer *structuredBuffer = new StructuredBuffer(structuredBufferResource, structuredBufferUploadResource, bufferResourceState, isRaw, accessType, uavDesc, uavHandle, srvHandle);
    structuredBuffer->SetIsReady(true);

    mBufferTracking[ContextTrackingType_StructuredBuffer]++;

    return structuredBuffer;
}

RenderTarget *Direct3DContextManager::CreateRenderTarget(uint32 width, uint32 height, DXGI_FORMAT format, bool hasUAV, uint16 arraySize, uint32 sampleCount, uint32 quality, uint32 mipLevels)
{
    Application::Assert(mipLevels > 0); //miplevels == 0 is valid for the desc, but it should always be possible to be explicit

	D3D12_RESOURCE_DESC targetDesc = {};
	targetDesc.Width = width;
	targetDesc.Height = height;
	targetDesc.Format = format;
	targetDesc.DepthOrArraySize = arraySize;
	targetDesc.MipLevels = mipLevels;
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

	RenderTarget::UAVHandle targetUAVHandle;
    targetUAVHandle.HasUAV = hasUAV;

	if (hasUAV)
	{
        targetUAVHandle.BaseHandle = mHeapManager->GetNewSRVDescriptorHeapHandle();
        mDevice->CreateUnorderedAccessView(renderTargetResource, NULL, NULL, targetUAVHandle.BaseHandle.GetCPUHandle());

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

        if (arraySize > 1)
        {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        }

        for (uint32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            for (uint32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
            {
                if (arraySize > 1)
                {
                    uavDesc.Texture2DArray.ArraySize = 1;
                    uavDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                    uavDesc.Texture2DArray.MipSlice = mipIndex;
                }
                else
                {
                    uavDesc.Texture2D.MipSlice = mipIndex;
                }
                
                DescriptorHeapHandle uavHandle = mHeapManager->GetNewSRVDescriptorHeapHandle();
                mDevice->CreateUnorderedAccessView(renderTargetResource, NULL, &uavDesc, uavHandle.GetCPUHandle());
                targetUAVHandle.Handles.Add(uavHandle);
            }
        }
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
		srvHandle, targetUAVHandle);

    mBufferTracking[ContextTrackingType_RenderTarget]++;

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
DepthStencilTarget *Direct3DContextManager::CreateDepthStencilTarget(uint32 width, uint32 height, DXGI_FORMAT format, uint16 arraySize, uint32 sampleCount, uint32 quality, float depthClearValue, uint8 stencilClearValue)
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
	clearValue.DepthStencil.Depth = depthClearValue;
	clearValue.DepthStencil.Stencil = stencilClearValue;

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

    mBufferTracking[ContextTrackingType_DepthStencil]++;

	return newDepthStencilTarget;
}

FilteredCubeMapRenderTexture *Direct3DContextManager::CreateFilteredCubeMapRenderTexture(uint32 dimensionSize, DXGI_FORMAT format, uint32 mipLevels)
{
    Application::Assert(mipLevels > 0);

    D3D12_RESOURCE_DESC targetDesc = {};
    targetDesc.Width = dimensionSize;
    targetDesc.Height = dimensionSize;
    targetDesc.Format = format;
    targetDesc.DepthOrArraySize = 6;
    targetDesc.MipLevels = mipLevels;
    targetDesc.Alignment = 0;
    targetDesc.SampleDesc.Count = 1;
    targetDesc.SampleDesc.Quality = 0;
    targetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    targetDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    targetDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES defaultHeapProperties;
    defaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    defaultHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    defaultHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    defaultHeapProperties.CreationNodeMask = 0;
    defaultHeapProperties.VisibleNodeMask = 0;

    ID3D12Resource *renderTextureResource = NULL;
    Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &targetDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL, IID_PPV_ARGS(&renderTextureResource)));

    DescriptorHeapHandle srvHandle = mHeapManager->GetNewSRVDescriptorHeapHandle();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = mipLevels;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

    mDevice->CreateShaderResourceView(renderTextureResource, &srvDesc, srvHandle.GetCPUHandle());

    DynamicArray<DescriptorHeapHandle> uavHandles;
    
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    uavDesc.Texture2DArray.ArraySize = 1;

    for (uint32 arrayIndex = 0; arrayIndex < 6; arrayIndex++)
    {
        for (uint32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
        {
            uavDesc.Texture2DArray.FirstArraySlice = arrayIndex;
            uavDesc.Texture2DArray.MipSlice = mipIndex;

            DescriptorHeapHandle uavHandle = mHeapManager->GetNewSRVDescriptorHeapHandle();
            mDevice->CreateUnorderedAccessView(renderTextureResource, NULL, &uavDesc, uavHandle.GetCPUHandle());
            uavHandles.Add(uavHandle);
        }
    }

    FilteredCubeMapRenderTexture *cubeTexture = new FilteredCubeMapRenderTexture(renderTextureResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, mipLevels, uavHandles, srvHandle);

    mBufferTracking[ContextTrackingType_FilteredCube]++;

    return cubeTexture;
}

RayTraceBuffer *Direct3DContextManager::CreateRayTraceBuffer(uint64 bufferSize, D3D12_RESOURCE_STATES initialState, RayTraceBuffer::RayTraceBufferType bufferType, bool hasSRV /*= false*/)
{
    if (bufferType == RayTraceBuffer::RayTraceBufferType_Transform)
    {
        bufferSize = MathHelper::AlignU64(bufferSize, D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT);
    }

    D3D12_RESOURCE_DESC bufferDesc;
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = bufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = bufferType == RayTraceBuffer::RayTraceBufferType_Acceleration_Structure ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES defaultHeapProperties;
    defaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    defaultHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    defaultHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    defaultHeapProperties.CreationNodeMask = 0;
    defaultHeapProperties.VisibleNodeMask = 0;

    D3D12_HEAP_PROPERTIES uploadHeapProperties;
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    uploadHeapProperties.CreationNodeMask = 0;
    uploadHeapProperties.VisibleNodeMask = 0;

    ID3D12Resource *bufferResource = NULL;

    switch (bufferType)
    {
    case RayTraceBuffer::RayTraceBufferType_Acceleration_Structure:
        Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, initialState, NULL, IID_PPV_ARGS(&bufferResource)));
        break;
    case RayTraceBuffer::RayTraceBufferType_Instancing:
    case RayTraceBuffer::RayTraceBufferType_Shader_Binding_Table_Storage:
        //state has to be generic read
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, initialState, NULL, IID_PPV_ARGS(&bufferResource)));
        
        void *mappedData;
        bufferResource->Map(0, NULL, reinterpret_cast<void**>(&mappedData));
        memset(mappedData, 0, bufferSize);
        bufferResource->Unmap(0, NULL);
        break;
    case RayTraceBuffer::RayTraceBufferType_Transform:
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        Direct3DUtils::ThrowIfHRESULTFailed(mDevice->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, initialState, NULL, IID_PPV_ARGS(&bufferResource)));
        break;
    default:
        Application::Assert(false);
    }

    DescriptorHeapHandle srvHandle = mHeapManager->GetNewSRVDescriptorHeapHandle();

    if (hasSRV)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.RaytracingAccelerationStructure.Location = bufferResource->GetGPUVirtualAddress();

        mDevice->CreateShaderResourceView(NULL, &srvDesc, srvHandle.GetCPUHandle());
    }

    RayTraceBuffer *rayTraceBuffer = new RayTraceBuffer(bufferResource, initialState, bufferType, srvHandle);
    rayTraceBuffer->SetIsReady(true); //readiness is later determined by the RayTraceManager since the creation requires GPU command work
    
    mBufferTracking[ContextTrackingType_RayTraceBuffer]++;

    return rayTraceBuffer;
}

void Direct3DContextManager::FreeRenderTarget(RenderTarget *renderTarget)
{
    Application::Assert(mBufferTracking[ContextTrackingType_RenderTarget] > 0);

	mHeapManager->FreeRTVDescriptorHeapHandle(renderTarget->GetRenderTargetViewHandle());
	mHeapManager->FreeSRVDescriptorHeapHandle(renderTarget->GetShaderResourceViewHandle());

	if (renderTarget->GetHasUAV())
	{
		mHeapManager->FreeSRVDescriptorHeapHandle(renderTarget->GetUnorderedAccessViewHandle()); //base UAV

        for (uint32 arrayIndex = 0; arrayIndex < renderTarget->GetArraySize(); arrayIndex++)
        {
            for (uint32 mipIndex = 0; mipIndex < renderTarget->GetMipCount(); mipIndex++)
            {
                mHeapManager->FreeSRVDescriptorHeapHandle(renderTarget->GetUnorderedAccessViewHandle(mipIndex, arrayIndex));
            }
        }
	}

	uint16 targetArraySize = renderTarget->GetArraySize() - 1;
	for (uint16 rtvArrayIndex = 0; rtvArrayIndex < targetArraySize; rtvArrayIndex++)
	{
		mHeapManager->FreeRTVDescriptorHeapHandle(renderTarget->GetRenderTargetViewHandle(rtvArrayIndex));
	}

	delete renderTarget;
    mBufferTracking[ContextTrackingType_RenderTarget]--;
}

void Direct3DContextManager::FreeDepthStencilTarget(DepthStencilTarget *depthStencilTarget)
{
    Application::Assert(mBufferTracking[ContextTrackingType_DepthStencil] > 0);

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
    mBufferTracking[ContextTrackingType_DepthStencil]--;
}

void Direct3DContextManager::FreeConstantBuffer(ConstantBuffer *constantBuffer)
{
    Application::Assert(mBufferTracking[ContextTrackingType_ConstantBuffer] > 0);

	mHeapManager->FreeSRVDescriptorHeapHandle(constantBuffer->GetConstantBufferViewHandle());
	delete constantBuffer;
    mBufferTracking[ContextTrackingType_ConstantBuffer]--;
}

void Direct3DContextManager::FreeStructuredBuffer(StructuredBuffer *structuredBuffer)
{
    Application::Assert(mBufferTracking[ContextTrackingType_StructuredBuffer] > 0);

    mHeapManager->FreeSRVDescriptorHeapHandle(structuredBuffer->GetUnorderedAccessViewHandle());
    mHeapManager->FreeSRVDescriptorHeapHandle(structuredBuffer->GetShaderResourceViewHandle());
    delete structuredBuffer;
    mBufferTracking[ContextTrackingType_StructuredBuffer]--;
}

void Direct3DContextManager::FreeFilteredCubeMap(FilteredCubeMapRenderTexture *cubeMapTexture)
{
    Application::Assert(mBufferTracking[ContextTrackingType_FilteredCube] > 0);

    for (uint32 arrayIndex = 0; arrayIndex < 6; arrayIndex++)
    {
        for (uint32 mipIndex = 0; mipIndex < cubeMapTexture->GetMipCount(); mipIndex++)
        {
            mHeapManager->FreeSRVDescriptorHeapHandle(cubeMapTexture->GetUnorderedAccessViewHandle(mipIndex, arrayIndex));
        }
    }

    mHeapManager->FreeSRVDescriptorHeapHandle(cubeMapTexture->GetShaderResourceViewHandle());
    delete cubeMapTexture;
    mBufferTracking[ContextTrackingType_FilteredCube]--;
}

void Direct3DContextManager::FreeRayTraceBuffer(RayTraceBuffer *rayTraceBuffer)
{
    Application::Assert(mBufferTracking[ContextTrackingType_RayTraceBuffer] > 0);

    mHeapManager->FreeSRVDescriptorHeapHandle(rayTraceBuffer->GetShaderResourceViewHandle());

    delete rayTraceBuffer;
    mBufferTracking[ContextTrackingType_RayTraceBuffer]--;
}