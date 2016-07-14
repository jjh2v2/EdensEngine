#pragma once
#include "Render/Shader/ShaderPipelineDefinition.h"

class ShaderPipelineStateCreator
{
public:
	ShaderPipelineStateCreator();
	~ShaderPipelineStateCreator();
private:
	CD3DX12_DEPTH_STENCIL_DESC mDepthStencilDescs[DepthStencil_MaxDepthStencilStates];
	CD3DX12_RASTERIZER_DESC mRasterDescs[Raster_MaxRasterStates];
	CD3DX12_BLEND_DESC mBlendDescs[Blend_MaxBlendStates];
};