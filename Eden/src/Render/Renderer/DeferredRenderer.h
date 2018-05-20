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
    void RenderShadows(D3DXMATRIX &lightViewMatrix, D3DXMATRIX &lightProjMatrix);
    void RenderLightingMain(const D3DXMATRIX &viewMatrix, const D3DXMATRIX &projectionMatrix, const D3DXMATRIX &viewToLightProjMatrix, const D3DXMATRIX &viewInvMatrix);
	void CopyToBackBuffer(RenderTarget *renderTargetToCopy);

	GraphicsManager *mGraphicsManager;

    SDSMShadowManager *mShadowManager;

	Scene *mActiveScene;

	DynamicArray<RenderTarget*> mGBufferTargets;
	DepthStencilTarget *mGBufferDepth;
    RenderTarget *mHDRTarget;

	ConstantBuffer *mCameraConstantBuffer[FRAME_BUFFER_COUNT];
    ConstantBuffer *mLightBuffer[FRAME_BUFFER_COUNT];

	SceneEntity *mSceneEntity;
	SceneEntity *mSceneEntity2;
	SceneEntity *mSceneEntity3;
    SceneEntity *mSceneEntity4;

    Texture *mSkyTexture;
    FilteredCubeMapRenderTexture *mFilteredCubeMap;
    RenderTarget *mEnvironmentMapLookup;

    uint64 mPreviousFrameFence;
    uint64 mGBufferPassFence;
};