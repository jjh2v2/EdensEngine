#pragma once

#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/Queue/Direct3DQueueManager.h"
#include "Render/Buffer/GPUBuffer.h"
#include "Render/Shader/ShaderPSO.h"
#include "Core/Misc/Color.h"
#include "Render/Shader/RootSignature/RootSignatureManager.h"
#include "Render/Texture/Texture.h"

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
	( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
	| D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
	| D3D12_RESOURCE_STATE_COPY_DEST \
	| D3D12_RESOURCE_STATE_COPY_SOURCE )

class Direct3DContext
{
public:
	Direct3DContext(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE commandType);
	virtual ~Direct3DContext();

	Direct3DContext(const Direct3DContext&) = delete;
	Direct3DContext & operator=(const Direct3DContext&) = delete;

	uint64 Flush(Direct3DQueueManager *queueManager, bool waitForCompletion = false);

	void FlushResourceBarriers();
	void BindDescriptorHeaps();
	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap *heap);
	void SetDescriptorHeaps(uint32 numHeaps, D3D12_DESCRIPTOR_HEAP_TYPE heapTypes[], ID3D12DescriptorHeap *heaps[]);

	void TransitionResource(GPUResource &resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
	void BeginResourceTransition(GPUResource &resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
	void InsertUAVBarrier(GPUResource &resource, bool flushImmediate = false);
	void InsertAliasBarrier(GPUResource &before, GPUResource &after, bool flushImmediate = false);

protected:
	D3D12_COMMAND_LIST_TYPE mContextType;
	ID3D12GraphicsCommandList* mCommandList;
	ID3D12CommandAllocator* mCommandAllocator;

	ID3D12RootSignature* mCurrentGraphicsRootSignature;
	ID3D12PipelineState* mCurrentGraphicsPipelineState;
	ID3D12RootSignature* mCurrentComputeRootSignature;
	ID3D12PipelineState* mCurrentComputePipelineState;

	D3D12_RESOURCE_BARRIER mResourceBarrierBuffer[16];
	uint32 mNumBarriersToFlush;

	ID3D12DescriptorHeap* mCurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};

class GraphicsContext : public Direct3DContext
{
public:
	GraphicsContext(ID3D12Device *device);
	virtual ~GraphicsContext();

	//void ClearUAV(GPUBuffer& Target);
	//void ClearUAV(ColorBuffer& Target);
	//void ClearColor(ColorBuffer& Target);
	//void ClearDepth(DepthBuffer& Target);
	//void ClearStencil(DepthBuffer& Target);
	//void ClearDepthAndStencil(DepthBuffer& Target);

	//void BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, uint32 HeapIndex);
	//void EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, uint32 HeapIndex);
	//void ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, uint32 StartIndex, uint32 NumQueries, ID3D12Resource* DestinationBuffer, uint3264 DestinationBufferOffset);

	void SetRootSignature(const RootSignatureInfo &rootSignature);

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
	void SetConstants(uint32 index, uint32 numConstants, const void *bufferData);
	void SetConstantBuffer(uint32 index, ConstantBuffer *constantBuffer);
	void SetTexture(uint32 index, Texture *texture, uint64 offset = 0);
	//void SetUnorderedAccessView(uint32 index, GPUBuffer &unorderedAccessView, uint64 offset = 0);

	void SetDescriptorTable(uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE handle);

	void SetIndexBuffer(IndexBuffer *indexBuffer);
	void SetVertexBuffer(uint32 slot, VertexBuffer *vertexBuffer);

	void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, float color[4]);

	void Draw(uint32 vertexCount, uint32 vertexStartOffset = 0);
	void DrawIndexed(uint32 indexCount, uint32 startIndexLocation = 0, int32 baseVertexLocation = 0);
	void DrawInstanced(uint32 vertexCountPerInstance, uint32 instanceCount,
		uint32 startVertexLocation = 0, uint32 startInstanceLocation = 0);
	void DrawIndexedInstanced(uint32 indexCountPerInstance, uint32 instanceCount, uint32 startIndexLocation,
		int32 baseVertexLocation, uint32 startInstanceLocation);
	//void DrawIndirect(GPUBuffer& argumentBuffer, size_t argumentBufferOffset = 0);

private:
};

struct Direct3DUploadInfo
{
	Direct3DUploadInfo()
	{
		CPUAddress = NULL;
		ResourceOffset = 0;
		Resource = NULL;
		UploadID = 0;
	}

	void* CPUAddress;
	uint64 ResourceOffset;
	ID3D12Resource* Resource;
	uint64 UploadID;
};

class UploadContext : public Direct3DContext
{
public:
	UploadContext(ID3D12Device *device);
	virtual ~UploadContext();
	
	Direct3DUploadInfo BeginUpload(uint64 size, Direct3DQueueManager *queueManager);
	void CopyTextureRegion(D3D12_TEXTURE_COPY_LOCATION *destination, D3D12_TEXTURE_COPY_LOCATION *source);
	void CopyResourceRegion(ID3D12Resource *destination, uint64 destOffset, ID3D12Resource *source, uint64 sourceOffset, uint64 numBytes);
	uint64 EndUpload(Direct3DUploadInfo& uploadInfo, Direct3DQueueManager *queueManager);
	
private:
	void ClearFinishedUploads(uint64 flushCount, Direct3DQueueManager *queueManager);
	bool AllocateUploadSubmission(uint64 size, uint64 &uploadIndex);
	

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

		ID3D12CommandAllocator *CommandAllocator;
		uint64 UploadLocation;
		uint64 UploadSize;
		uint64 UploadPadding;
		uint64 FenceValue;
		bool   IsUploading;
	};

	UploadBuffer mUploadBuffer;
	Direct3DUpload mUploads[MAX_GPU_UPLOADS];
	uint64 mUploadSubmissionStart;
	uint64 mUploadSubmissionUsed;
};