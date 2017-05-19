#pragma once
#include "Render/Renderer/DeferredRenderer.h"
#include "Render/Graphics/GraphicsManager.h"
#include "Scene/Scene.h"

/*
struct MatrixBufferTest
{
	D3DXMATRIX worldMatrix;
	D3DXMATRIX viewMatrix;
	D3DXMATRIX projectionMatrix;
};
*/

class DeferredRenderer
{
public:
	DeferredRenderer(GraphicsManager *graphicsManager);
	~DeferredRenderer();

	void SetActiveScene(Scene *scene) { mActiveScene = scene; }
	void Render();

private:
	void ClearGBuffer();

	GraphicsManager *mGraphicsManager;
	Scene *mActiveScene;

	DynamicArray<RenderTarget*> mGBufferTargets;
	DepthStencilTarget *mGBufferDepth;

	RenderPassDescriptorHeap *mGBufferCBVDescHeap;
	RenderPassDescriptorHeap *mGBufferSamplerDescHeap;

	DescriptorHeapHandle mCameraBufferStart;
	DescriptorHeapHandle mMaterialBufferStart;
	DescriptorHeapHandle mTextureStart;
	DescriptorHeapHandle mSamplerStart;
	DescriptorHeapHandle mCopyTextureStart;
	DescriptorHeapHandle mCopySamplerStart;
	ConstantBuffer *mCameraConstantBuffer;
	ConstantBuffer *mMaterialConstantBuffer;

	Sampler *mSampler;
	Texture *mTexture;
	Mesh *mMesh;

};