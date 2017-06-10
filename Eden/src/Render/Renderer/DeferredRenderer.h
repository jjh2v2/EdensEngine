#pragma once
#include "Render/Renderer/DeferredRenderer.h"
#include "Render/Graphics/GraphicsManager.h"
#include "Scene/Scene.h"

#define GBufferTextureInputCount 3

class DeferredRenderer
{
public:
	DeferredRenderer(GraphicsManager *graphicsManager);
	~DeferredRenderer();

	void SetActiveScene(Scene *scene) { mActiveScene = scene; }
	void Render();

private:
	void ClearGBuffer();
	void RenderGBuffer();
	void CopyToBackBuffer(RenderTarget *renderTargetToCopy);

	GraphicsManager *mGraphicsManager;
	Scene *mActiveScene;

	DynamicArray<RenderTarget*> mGBufferTargets;
	DepthStencilTarget *mGBufferDepth;

	RenderPassDescriptorHeap *mGBufferCBVDescHeap;
	RenderPassDescriptorHeap *mGBufferSamplerDescHeap;

	ConstantBuffer *mCameraConstantBuffer;

	SceneEntity *mSceneEntity;
	SceneEntity *mSceneEntity2;
	SceneEntity *mSceneEntity3;
};