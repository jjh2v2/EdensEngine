#pragma once
#include "Render/Renderer/DeferredRenderer.h"
#include "Render/Graphics/GraphicsManager.h"

class DeferredRenderer
{
public:
	DeferredRenderer(GraphicsManager *graphicsManager);
	~DeferredRenderer();

private:
	GraphicsManager *mGraphicsManager;

	ID3D12GraphicsCommandList *mCommandList;
	ID3D12RootSignature	*mRootSignature;
	ID3D12PipelineState	*mPipelineState;
};