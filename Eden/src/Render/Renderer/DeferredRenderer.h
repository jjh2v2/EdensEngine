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

	RenderPassDescriptorHeap *mGBufferTextureDescHeap;
	RenderPassDescriptorHeap *mGBufferCBVDescHeap;
	RenderPassDescriptorHeap *mGBufferPerFrameDescHeap;
	RenderPassDescriptorHeap *mGBufferSamplerDescHeap;

	//Mesh *mMesh;
	//Material *mMaterial;
};