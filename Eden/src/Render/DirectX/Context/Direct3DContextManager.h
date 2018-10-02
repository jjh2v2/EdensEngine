#pragma once

#include "Render/DirectX/Context/Direct3DContext.h"
#include "Render/DirectX/Queue/Direct3DQueueManager.h"
#include "Render/DirectX/Heap/Direct3DHeapManager.h"

class Direct3DContextManager
{
public:
	Direct3DContextManager(ID3D12Device* device, bool isRayTracingSupported);
	~Direct3DContextManager();
    void FinishContextsAndWaitForIdle();

	Direct3DHeapManager *GetHeapManager() { return mHeapManager; }

	Direct3DQueueManager *GetQueueManager() { return mQueueManager; }
	GraphicsContext *GetGraphicsContext() { return mGraphicsContext; }
    ComputeContext *GetComputeContext() { return mComputeContext; }
	UploadContext *GetUploadContext() { return mUploadContext; }
    RayTraceContext *GetRayTraceContext() { return mRayTraceContext; }

	RenderTarget *CreateRenderTarget(uint32 width, uint32 height, DXGI_FORMAT format, bool hasUAV, uint16 arraySize, uint32 sampleCount, uint32 quality, uint32 mipLevels = 1);
	DepthStencilTarget *CreateDepthStencilTarget(uint32 width, uint32 height, DXGI_FORMAT format, uint16 arraySize, uint32 sampleCount, uint32 quality, float depthClearValue = 0.0f, uint8 stencilClearValue = 0);
	VertexBuffer *CreateVertexBuffer(void* vertexData, uint32 vertexStride, uint32 bufferSize);
	IndexBuffer *CreateIndexBuffer(void* indexData, uint32 bufferSize);
	ConstantBuffer *CreateConstantBuffer(uint32 bufferSize);
    StructuredBuffer *CreateStructuredBuffer(uint32 elementSize, uint32 numElements, StructuredBufferAccess accessType, bool isRaw);
    FilteredCubeMapRenderTexture *CreateFilteredCubeMapRenderTexture(uint32 dimensionSize, DXGI_FORMAT format, uint32 mipLevels);

    RayTraceBuffer *CreateRayTraceBuffer(uint64 bufferSize, D3D12_RESOURCE_STATES initialState, RayTraceBuffer::RayTraceBufferType bufferType, bool hasSRV = false);

	void FreeRenderTarget(RenderTarget *renderTarget);
	void FreeDepthStencilTarget(DepthStencilTarget *depthStencilTarget);
	void FreeConstantBuffer(ConstantBuffer *constantBuffer);
    void FreeStructuredBuffer(StructuredBuffer *structuredBuffer);
    void FreeFilteredCubeMap(FilteredCubeMapRenderTexture *cubeMapTexture);
    void FreeRayTraceBuffer(RayTraceBuffer *rayTraceBuffer);

private:
    class VertexBufferBackgroundUpload : public UploadContext::BackgroundUpload
    {
    public:
        VertexBufferBackgroundUpload(Direct3DContextManager *contextManager, VertexBuffer *vertexBuffer, void *vertexData);
        virtual void ProcessUpload(UploadContext *uploadContext);
        virtual void OnUploadFinished();

    private:
        Direct3DContextManager *mContextManager;
        VertexBuffer *mVertexBuffer;
        void *mVertexData;
    };

    class IndexBufferBackgroundUpload : public UploadContext::BackgroundUpload
    {
    public:
        IndexBufferBackgroundUpload(Direct3DContextManager *contextManager, IndexBuffer *indexBuffer, void *indexData);
        virtual void ProcessUpload(UploadContext *uploadContext);
        virtual void OnUploadFinished();

    private:
        Direct3DContextManager *mContextManager;
        IndexBuffer *mIndexBuffer;
        void *mIndexData;
    };

    enum ContextTrackingType
    {
        ContextTrackingType_RenderTarget = 0,
        ContextTrackingType_DepthStencil,
        ContextTrackingType_ConstantBuffer,
        ContextTrackingType_StructuredBuffer,
        ContextTrackingType_FilteredCube,
        ContextTrackingType_RayTraceBuffer,
        ContextTrackingType_Max
    };

	ID3D12Device *mDevice;
	Direct3DHeapManager *mHeapManager;
	Direct3DQueueManager *mQueueManager;
	GraphicsContext *mGraphicsContext;
    ComputeContext *mComputeContext;
	UploadContext *mUploadContext;
    RayTraceContext *mRayTraceContext;

    uint32 mBufferTracking[ContextTrackingType_Max];
};