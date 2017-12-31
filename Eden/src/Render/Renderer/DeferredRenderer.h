#pragma once
#include "Render/Renderer/DeferredRenderer.h"
#include "Render/Graphics/GraphicsManager.h"
#include "Scene/Scene.h"

#define GBufferTextureInputCount 3

class SDSMShadowManager;

class DeferredRenderer
{
public:
	DeferredRenderer(GraphicsManager *graphicsManager);
	~DeferredRenderer();

	void OnScreenChanged(Vector2 screenSize);
	void SetActiveScene(Scene *scene) { mActiveScene = scene; }
	void Render();

private:
	void FreeTargets();
	void CreateTargets(Vector2 screenSize);

	void ClearGBuffer();
	void RenderGBuffer();
	void CopyToBackBuffer(RenderTarget *renderTargetToCopy);

	GraphicsManager *mGraphicsManager;

    SDSMShadowManager *mShadowManager;

	Scene *mActiveScene;

	DynamicArray<RenderTarget*> mGBufferTargets;
	DepthStencilTarget *mGBufferDepth;

	ConstantBuffer *mCameraConstantBuffer[FRAME_BUFFER_COUNT];

	SceneEntity *mSceneEntity;
	SceneEntity *mSceneEntity2;
	SceneEntity *mSceneEntity3;

    uint64 mPreviousFrameFence;
};