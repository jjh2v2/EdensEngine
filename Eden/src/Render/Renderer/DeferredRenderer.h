#pragma once
#include "Render/Renderer/DeferredRenderer.h"
#include "Render/Graphics/GraphicsManager.h"
#include "Scene/Scene.h"

struct MatrixBufferTest
{
	D3DXMATRIX worldMatrix;
	D3DXMATRIX viewMatrix;
	D3DXMATRIX projectionMatrix;
};

class DeferredRenderer
{
public:
	DeferredRenderer(GraphicsManager *graphicsManager);
	~DeferredRenderer();

	void SetActiveScene(Scene *scene) { mActiveScene = scene; }
	void Render();

private:
	GraphicsManager *mGraphicsManager;
	Scene *mActiveScene;

	DynamicArray<RenderTarget*> mGBufferTargets;
	DepthStencilTarget *mGBufferDepth;

	RenderPassDescriptorHeap *mGBufferCBVDescHeap;
	RenderPassDescriptorHeap *mGBufferSamplerDescHeap;

	DescriptorHeapHandle mMatrixBufferStart;
	DescriptorHeapHandle mTextureStart;
	DescriptorHeapHandle mSamplerStart;
	DescriptorHeapHandle mCopyTextureStart;
	DescriptorHeapHandle mCopySamplerStart;
	ConstantBuffer *mMatrixConstantBuffer;
	ShaderTechnique *mShader;
	Sampler *mSampler;
	Texture *mTexture;
	Mesh *mMesh;

};