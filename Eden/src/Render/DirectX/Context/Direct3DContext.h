#pragma once

#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/Queue/Direct3DQueueManager.h"
#include "Render/Buffer/GPUBuffer.h"
#include "Render/Shader/ShaderPSO.h"
#include "Core/Misc/Color.h"
#include "Render/Shader/RootSignature/RootSignatureManager.h"

class Direct3DContext
{
public:
	Direct3DContext(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE commandType);
	~Direct3DContext();

	Direct3DContext(const Direct3DContext&) = delete;
	Direct3DContext & operator=(const Direct3DContext&) = delete;

	uint64 Flush(Direct3DQueueManager *queueManager, bool waitForCompletion = false);
	void FlushResourceBarriers();
	void BindDescriptorHeaps();

	ID3D12GraphicsCommandList *GetCommandList(){ return mCommandList; }

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap *heap);
	void SetDescriptorHeaps(uint32 numHeaps, D3D12_DESCRIPTOR_HEAP_TYPE heapTypes[], ID3D12DescriptorHeap *heaps[]);

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

	//LinearAllocator m_CpuLinearAllocator;
	//LinearAllocator m_GpuLinearAllocator;
};

class GraphicsContext : public Direct3DContext
{
public:

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

	void SetPipelineState(ShaderPSO &pipeline);
	void SetConstants(uint32 index, uint32 numConstants, const void *bufferData);
	void SetConstantBuffer(uint32 index, D3D12_GPU_VIRTUAL_ADDRESS constantBuffer);
	void SetShaderResourceView(uint32 index, GPUBuffer &shaderResourceView, uint64 offset = 0);
	void SetUnorderedAccessView(uint32 index, GPUBuffer &unorderedAccessView, uint64 offset = 0);

	void SetDescriptorTable(uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE handle);

	void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW &indexBuffer);
	void SetVertexBuffer(uint32 slot, const D3D12_VERTEX_BUFFER_VIEW &vertexBuffer);
	void SetVertexBuffers(uint32 slot, uint32 count, const D3D12_VERTEX_BUFFER_VIEW vertexBuffers[]);

	void Draw(uint32 vertexCount, uint32 vertexStartOffset = 0);
	void DrawIndexed(uint32 indexCount, uint32 startIndexLocation = 0, int32 baseVertexLocation = 0);
	void DrawInstanced(uint32 vertexCountPerInstance, uint32 instanceCount,
		uint32 startVertexLocation = 0, uint32 startInstanceLocation = 0);
	void DrawIndexedInstanced(uint32 indexCountPerInstance, uint32 instanceCount, uint32 startIndexLocation,
		int32 baseVertexLocation, uint32 startInstanceLocation);
	void DrawIndirect(GPUBuffer& argumentBuffer, size_t argumentBufferOffset = 0);

private:
};