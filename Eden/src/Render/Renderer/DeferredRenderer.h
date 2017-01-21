#pragma once
#include "Render/Renderer/DeferredRenderer.h"
#include "Render/Graphics/GraphicsManager.h"

class DeferredRenderer
{
public:
	DeferredRenderer(GraphicsManager *graphicsManager);
	~DeferredRenderer();

	void Render();

private:
	GraphicsManager *mGraphicsManager;

	DynamicArray<RenderTarget*> mGBufferTargets;
	DepthStencilTarget *mGBufferDepth;

	RenderPassDescriptorHeap *mGBufferTextureDescHeap;
	RenderPassDescriptorHeap *mGBufferCBVDescHeap;
	RenderPassDescriptorHeap *mGBufferPerFrameDescHeap;
	RenderPassDescriptorHeap *mGBufferSamplerDescHeap;

	ShaderTechnique *mShader;
	Sampler *mSampler;
	Texture *mTexture;
	Mesh *mMesh;
};