#pragma once

#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/Queue/Direct3DQueueManager.h"
#include "Render/Shader/ShaderPSO.h"
#include "Core/Misc/Color.h"
#include "Render/Shader/RootSignature/RootSignatureManager.h"
#include "Render/Texture/Texture.h"
#include "Render/DirectX/Heap/DescriptorHeap.h"
#include "Render/Shader/Definitions/MaterialDefinitions.h"

class Direct3DCommandAllocatorPool;

class Direct3DContext
{
public:
	Direct3DContext(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE commandType);
	virtual ~Direct3DContext();

	Direct3DContext(const Direct3DContext&) = delete;
	Direct3DContext & operator=(const Direct3DContext&) = delete;

    void WaitForCommandIdle(Direct3DQueueManager *queueManager);
	uint64 Flush(Direct3DQueueManager *queueManager, bool waitForCompletion = false, bool finalize = false);

    void FlushDeferredTransitions();
	void FlushResourceBarriers();

	void BindDescriptorHeaps();
	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap *heap);
	void SetDescriptorHeaps(uint32 numHeaps, D3D12_DESCRIPTOR_HEAP_TYPE heapTypes[], ID3D12DescriptorHeap *heaps[]);
	void CopyDescriptors(uint32 numDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE destinationStart, D3D12_CPU_DESCRIPTOR_HANDLE sourceStart, D3D12_DESCRIPTOR_HEAP_TYPE heapType);

	void TransitionResource(GPUResource *resource, D3D12_RESOURCE_STATES newState, bool flushTransitionsImmediate = false);
    void TransitionResourceExplicit(GPUResource *resource, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState, uint32 subresourceIndex, bool setResourceUsageState, bool flushTransitionsImmediate);
    void TransitionResourceDeferred(GPUResource *resource, D3D12_RESOURCE_STATES newState, bool setResourceReady = false);
	void InsertUAVBarrier(GPUResource *resource, bool flushImmediate = false);
	void InsertAliasBarrier(GPUResource *before, GPUResource *after, bool flushImmediate = false);

    void InsertPixBeginEvent(uint64 color, const char* markerString);
    void InsertPixEndEvent();

protected:
    struct DeferredTransition
    {
        GPUResource *Resource;
        D3D12_RESOURCE_STATES NewState;
        bool SetResourceReady;
    };

	ID3D12Device *mDevice;

	D3D12_COMMAND_LIST_TYPE mContextType;
	ID3D12GraphicsCommandList *mCommandList;

    Direct3DCommandAllocatorPool *mCommandAllocatorPool;
	ID3D12CommandAllocator *mCurrentCommandAllocator;

	ID3D12RootSignature *mCurrentGraphicsRootSignature;
	ID3D12PipelineState *mCurrentGraphicsPipelineState;
	ID3D12RootSignature *mCurrentComputeRootSignature;
	ID3D12PipelineState *mCurrentComputePipelineState;

	D3D12_RESOURCE_BARRIER mResourceBarrierBuffer[16];
	uint32 mNumBarriersToFlush;

	ID3D12DescriptorHeap* mCurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    DynamicArray<DeferredTransition> mDeferredTransitions;
    std::mutex mDeferredTransitionArrayMutex;
};

class GraphicsContext : public Direct3DContext
{
public:
	GraphicsContext(ID3D12Device *device);
	virtual ~GraphicsContext();

	void SetRootSignature(ID3D12RootSignature *graphicsRootSignature, ID3D12RootSignature *computeRootSignature);

	void SetRenderTargets(uint32 numRenderTargets, const D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[]);
	void SetRenderTargets(uint32 numRenderTargets, const D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[], D3D12_CPU_DESCRIPTOR_HANDLE depthStencil);
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE renderTarget) { SetRenderTargets(1, &renderTarget); }
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE renderTarget, D3D12_CPU_DESCRIPTOR_HANDLE depthStencil) { SetRenderTargets(1, &renderTarget, depthStencil); }
	void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE depthStencil) { SetRenderTargets(0, NULL, depthStencil); }

	void SetViewport(const D3D12_VIEWPORT &viewPort);
	void SetViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f);
	void SetScissorRect(const D3D12_RECT &rect);
	void SetScissorRect(uint32 left, uint32 top, uint32 right, uint32 bottom);
	void SetStencilRef(uint32 stencilRef);
	void SetBlendFactor(Color blendFactor);
	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);

	void SetPipelineState(ShaderPSO *pipeline);
	void SetGraphicsConstants(uint32 index, uint32 numConstants, const void *bufferData);
	void SetGraphicsRootConstantBuffer(uint32 index, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
    void SetGraphicsRootShaderResourceView(uint32 index, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
    void SetGraphicsRootUnorderedAccessView(uint32 index, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
	void SetGraphicsDescriptorTable(uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE handle);

	void SetIndexBuffer(IndexBuffer *indexBuffer);
	void SetVertexBuffer(uint32 slot, VertexBuffer *vertexBuffer);

	void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, float color[4]);
	void ClearDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, float depth, uint8 stencil);
    void ClearUnorderedAccessView(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandleInHeap, D3D12_CPU_DESCRIPTOR_HANDLE target, ID3D12Resource *targetResource, uint32 values[4]);

	void DrawFullScreenTriangle();
	void Draw(uint32 vertexCount, uint32 vertexStartOffset = 0);
	void DrawIndexed(uint32 indexCount, uint32 startIndexLocation = 0, int32 baseVertexLocation = 0);
	void DrawInstanced(uint32 vertexCountPerInstance, uint32 instanceCount,
		uint32 startVertexLocation = 0, uint32 startInstanceLocation = 0);
	void DrawIndexedInstanced(uint32 indexCountPerInstance, uint32 instanceCount, uint32 startIndexLocation,
		int32 baseVertexLocation, uint32 startInstanceLocation);

    void SetComputeConstants(uint32 index, uint32 numConstants, const void *bufferData);
    void SetComputeRootConstantBuffer(uint32 index, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
    void SetComputeRootShaderResourceView(uint32 index, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
    void SetComputeRootUnorderedAccessView(uint32 index, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
    void SetComputeDescriptorTable(uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE handle);

    void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ);
    void Dispatch1D(uint32 threadCountX, uint32 groupSizeX);
    void Dispatch2D(uint32 threadCountX, uint32 threadCountY, uint32 groupSizeX, uint32 groupSizeY);
    void Dispatch3D(uint32 threadCountX, uint32 threadCountY, uint32 threadCountZ, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ);

    void CopyResource(ID3D12Resource *destination, ID3D12Resource *source);
    void CopyResourceRegion(ID3D12Resource *destination, uint64 destOffset, ID3D12Resource *source, uint64 sourceOffset, uint64 numBytes);

private:
};

class ComputeContext : public Direct3DContext
{
public:
    ComputeContext(ID3D12Device *device);
    virtual ~ComputeContext();

    void SetRootSignature(ID3D12RootSignature *rootSignature);
    void SetPipelineState(ShaderPSO *pipeline);
    void SetConstants(uint32 index, uint32 numConstants, const void *bufferData);
    void SetRootConstantBuffer(uint32 index, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
    void SetDescriptorTable(uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE handle);

    void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ);
    void Dispatch1D(uint32 threadCountX, uint32 groupSizeX);
    void Dispatch2D(uint32 threadCountX, uint32 threadCountY, uint32 groupSizeX, uint32 groupSizeY);
    void Dispatch3D(uint32 threadCountX, uint32 threadCountY, uint32 threadCountZ, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ);

private:
};


struct Direct3DUploadInfo
{
	Direct3DUploadInfo()
	{
		CPUAddress = NULL;
		UploadAddressOffset = 0;
		Resource = NULL;
		UploadID = 0;
	}

	void *CPUAddress;
	uint64 UploadAddressOffset;
	ID3D12Resource *Resource;
	uint64 UploadID;
};

class UploadContext : public Direct3DContext
{
public:
    class BackgroundUpload
    {
    public:
        BackgroundUpload() { mUploadFence = 0; }
        virtual void ProcessUpload(UploadContext *uploadContext) = 0;
        virtual void OnUploadFinished() {}

        uint64 GetUploadFence() { return mUploadFence; }

    protected:
        uint64 mUploadFence;
    };

	UploadContext(ID3D12Device *device);
	virtual ~UploadContext();
	
    void ProcessBackgroundUploads(uint32 maxToProcess);
    void ProcessFinishedUploads(uint64 mostRecentFence);
    void AddBackgroundUpload(BackgroundUpload *backgroundUpload);
	
    void LockUploads() { mUploadProcessMutex.lock(); }
    void UnlockUploads() { mUploadProcessMutex.unlock(); }

    //If these functions are used outside of BackgroundUploads, it must call Lock/UnlockUploads to be safe
    Direct3DUploadInfo BeginUpload(uint64 size, Direct3DQueueManager *queueManager);
    void CopyTextureRegion(D3D12_TEXTURE_COPY_LOCATION *destination, D3D12_TEXTURE_COPY_LOCATION *source);
    void CopyResourceRegion(ID3D12Resource *destination, uint64 destOffset, ID3D12Resource *source, uint64 sourceOffset, uint64 numBytes);
    uint64 FlushUpload(Direct3DUploadInfo& uploadInfo, Direct3DQueueManager *queueManager, bool forceWait = false);

private:

    struct UploadBuffer
    {
        UploadBuffer()
        {
            BufferResource = NULL;
            BufferAddress = NULL;
            BufferStart = 0;
            BufferUsed = 0;
        }

        ID3D12Resource *BufferResource;
        uint8 *BufferAddress;
        uint64 BufferStart;
        uint64 BufferUsed;
    };

    struct Direct3DUpload
    {
        Direct3DUpload()
        {
            Reset();
        }

        void Reset()
        {
            UploadLocation = 0;
            UploadSize = 0;
            UploadPadding = 0;
            FenceValue = UINT64_MAX;
            IsUploading = false;
        }

        uint64 UploadLocation;
        uint64 UploadSize;
        uint64 UploadPadding;
        uint64 FenceValue;
        bool   IsUploading;
    };

    bool ClearSubmissionIfFinished(Direct3DUpload &submission, Direct3DQueueManager *queueManager);
	uint32 ClearFinishedUploads(uint64 flushCount, Direct3DQueueManager *queueManager);
	bool CreateNewUpload(uint64 size, uint32 &uploadIndex);
	
	UploadBuffer mUploadBuffer;
	Direct3DUpload mUploads[MAX_GPU_UPLOADS];
	uint32 mUploadSubmissionStart;
	uint32 mUploadSubmissionUsed;

    DynamicArray<BackgroundUpload*> mBackgroundUploadsToProcess;
    DynamicArray<BackgroundUpload*> mBackgroundUploadsInFlight;

    std::mutex mBackgroundUploadArrayMutex;
    std::mutex mUploadProcessMutex; //specifically for the purpose of allowing safe synchronous uploads while the background uploads are potentially processing
};

class RayTraceContext : public Direct3DContext
{
public:
    RayTraceContext(ID3D12Device *device);
    virtual ~RayTraceContext();

    void SetRayPipelineState(ID3D12StateObject *rayTraceStateObject);
    void BuildAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC &structureDesc);
    void DispatchRays(D3D12_DISPATCH_RAYS_DESC &dispatchDesc);
    void SetComputeDescriptorTable(uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE handle);
    void SetComputeRootSignature(ID3D12RootSignature *computeRootSignature);

private:
    ID3D12GraphicsCommandList4 *mDXRCommandList;
};

class RenderPassContext
{
public:
	RenderPassContext(GraphicsContext *context, RenderPassDescriptorHeap *cbvSrvHeap, RenderPassDescriptorHeap *samplerHeap, DynamicArray<MaterialTextureType> &renderPassTextures, uint32 frameIndex)
		:mGraphicsContext(context)
		,mCBVSRVHeap(cbvSrvHeap)
        ,mSamplerHeap(samplerHeap)
        ,mFrameIndex(frameIndex)
	{
		for (uint32 i = 0; i < renderPassTextures.CurrentSize(); i++)
		{
			mRenderPassTextures.Add(renderPassTextures[i]);
		}
	}

	GraphicsContext *GetGraphicsContext() { return mGraphicsContext; }
	RenderPassDescriptorHeap *GetCBVSRVHeap() { return mCBVSRVHeap; }
    RenderPassDescriptorHeap *GetSamplerHeap() { return mSamplerHeap; }

	MaterialTextureType GetRenderPassTextureType(uint32 index) { return mRenderPassTextures[index]; }
	uint32 GetRenderPassTextureCount() { return mRenderPassTextures.CurrentSize(); }
    uint32 GetFrameIndex() { return mFrameIndex; }

private:
	GraphicsContext *mGraphicsContext;
	RenderPassDescriptorHeap *mCBVSRVHeap;
    RenderPassDescriptorHeap *mSamplerHeap;
	DynamicArray<MaterialTextureType> mRenderPassTextures;
    uint32 mFrameIndex;
};