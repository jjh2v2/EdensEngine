#pragma once
#include "Render/Renderer/DeferredRenderer.h"
#include "Render/Graphics/GraphicsManager.h"
#include "Scene/Scene.h"

#define GBufferTextureInputCount 3

class SDSMShadowManager;
class PostProcessManager;

class DeferredRenderer
{
public:
	DeferredRenderer(GraphicsManager *graphicsManager);
	~DeferredRenderer();

	void OnScreenChanged(Vector2 screenSize);
	void SetActiveScene(Scene *scene) { mActiveScene = scene; }
	void Render(float deltaTime);
    void ToggleRayTraceOutput(bool showRayTrace) { mShowRayTrace = showRayTrace; }
    void ToggleTonemapper();
    void ToggleRayShadows();

private:
	void FreeTargets();
	void CreateTargets(Vector2 screenSize);

    void RenderRayTracing();
    void ProcessRayShadows();
	void ClearFrameBuffers();
	void RenderGBuffer();
    void RenderSky();
    void RenderShadows(D3DXMATRIX &lightViewMatrix, D3DXMATRIX &lightProjMatrix);
    void RenderLightingMain(const D3DXMATRIX &viewMatrix, const D3DXMATRIX &projectionMatrix, const D3DXMATRIX &viewToLightProjMatrix, const D3DXMATRIX &viewInvMatrix);
    void RenderWater(float deltaTime);

	GraphicsManager *mGraphicsManager;

    SDSMShadowManager *mShadowManager;
    PostProcessManager *mPostProcessManager;
    RayTraceManager *mRayTraceManager;

	Scene *mActiveScene;

	DynamicArray<RenderTarget*> mGBufferTargets;
	DepthStencilTarget *mGBufferDepth;
    RenderTarget *mHDRTarget;
    RenderTarget *mRayShadowBlurTarget;

	ConstantBuffer *mCameraConstantBuffer[FRAME_BUFFER_COUNT];
    ConstantBuffer *mSkyBuffer[FRAME_BUFFER_COUNT];
    ConstantBuffer *mLightBuffer[FRAME_BUFFER_COUNT];
    ConstantBuffer *mRayShadowBlurBuffer[FRAME_BUFFER_COUNT];

	SceneEntity *mSceneEntity;
	SceneEntity *mSceneEntity2;
	SceneEntity *mSceneEntity3;
    SceneEntity *mSceneEntity4;

    ConstantBuffer *mWaterBuffer[FRAME_BUFFER_COUNT];
    Mesh *mWaterMesh;
    Texture *mWaterHeightMap1;
    Texture *mWaterHeightMap2;
    Texture *mWaterNormalMap;

    Texture *mSkyTexture;
    Mesh *mSkyMesh;

    FilteredCubeMapRenderTexture *mFilteredCubeMap;
    RenderTarget *mEnvironmentMapLookup;

    uint64 mPreviousFrameFence;
    bool mMeshesAddedToRayTrace;
    bool mShowRayTrace;
    bool mUseRayTraceShadows;
};