#include "Render/Renderer/DeferredRenderer.h"

DeferredRenderer::DeferredRenderer(GraphicsManager *graphicsManager)
{
	mGraphicsManager = graphicsManager;
	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	direct3DManager->WaitForGPU();

	//we need to be able to make these bigger
	mGBufferTextureDescHeap = new RenderPassDescriptorHeap(direct3DManager->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024, true);
	mGBufferCBVDescHeap = new RenderPassDescriptorHeap(direct3DManager->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024, true);
	mGBufferPerFrameDescHeap = new RenderPassDescriptorHeap(direct3DManager->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16, true);
	mGBufferSamplerDescHeap = new RenderPassDescriptorHeap(direct3DManager->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 16, true);
}

DeferredRenderer::~DeferredRenderer()
{
	delete mGBufferTextureDescHeap;
	delete mGBufferCBVDescHeap;
	delete mGBufferPerFrameDescHeap;
	delete mGBufferSamplerDescHeap;
}

void DeferredRenderer::Render()
{
	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();

	graphicsContext->SetViewport(direct3DManager->GetScreenViewport());

	BackBufferTarget *backBuffer = direct3DManager->GetBackBufferTarget();
	graphicsContext->TransitionResource((*backBuffer), D3D12_RESOURCE_STATE_RENDER_TARGET, true);

	static float blueSub = 0.0f;
	if (blueSub < 0.9f)
	{
		blueSub += 0.0001f;
	}
	
	// Record drawing commands.
	float color[4] = {0.392156899f, 0.584313750f, 0.929411829f - blueSub, 1.000000000f};
	graphicsContext->ClearRenderTarget(direct3DManager->GetRenderTargetView(), color);

	graphicsContext->TransitionResource((*backBuffer), D3D12_RESOURCE_STATE_PRESENT, true);

	graphicsContext->Flush(direct3DManager->GetContextManager()->GetQueueManager(), true);
}