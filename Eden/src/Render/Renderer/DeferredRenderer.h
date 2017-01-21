#pragma once
#include "Render/Renderer/DeferredRenderer.h"
#include "Render/Graphics/GraphicsManager.h"
#include "Camera/Camera.h"

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

	void Render();

private:
	GraphicsManager *mGraphicsManager;

	DynamicArray<RenderTarget*> mGBufferTargets;
	DepthStencilTarget *mGBufferDepth;

	RenderPassDescriptorHeap *mGBufferTextureDescHeap;
	RenderPassDescriptorHeap *mGBufferCBVDescHeap;
	RenderPassDescriptorHeap *mGBufferPerFrameDescHeap;
	RenderPassDescriptorHeap *mGBufferSamplerDescHeap;

	Camera *mCamera;

	DescriptorHeapHandle mMatrixBufferStart;
	DescriptorHeapHandle mTextureStart;
	DescriptorHeapHandle mSamplerStart;
	ConstantBuffer *mMatrixConstantBuffer;
	ShaderTechnique *mShader;
	Sampler *mSampler;
	Texture *mTexture;
	Mesh *mMesh;

};