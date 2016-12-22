#include "Render/Renderer/DeferredRenderer.h"

DeferredRenderer::DeferredRenderer(GraphicsManager *graphicsManager)
{
	mGraphicsManager = graphicsManager;
	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	direct3DManager->WaitForGPU();
}

DeferredRenderer::~DeferredRenderer()
{

}

void DeferredRenderer::Render()
{
	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();

	graphicsContext->SetViewport(direct3DManager->GetScreenViewport());

	RenderTarget *backBuffer = direct3DManager->GetBackBufferTarget();
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