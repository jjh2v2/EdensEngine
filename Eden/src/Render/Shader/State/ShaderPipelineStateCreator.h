#pragma once
#include "Render/Shader/ShaderPipelineDefinition.h"
#include <map>

class ShaderPipelineStateCreator
{
public:
	ShaderPipelineStateCreator();
	~ShaderPipelineStateCreator();

	ShaderPipelineRenderState GetPipelineRenderState(uint32 renderStateID) { return mPipelineRenderStateMap[renderStateID]; }
	ShaderPipelineTargetState GetPipelineTargetState(ShaderTargetStateType stateType) { return mPipelineTargetStateMap[stateType]; }

private:
	CD3DX12_DEPTH_STENCIL_DESC mDepthStencilDescs[DepthStencil_MaxDepthStencilStates];
	CD3DX12_RASTERIZER_DESC mRasterDescs[Raster_MaxRasterStates];
	CD3DX12_BLEND_DESC mBlendDescs[Blend_MaxBlendStates];

	std::map<uint32, ShaderPipelineRenderState> mPipelineRenderStateMap;
	std::map<ShaderTargetStateType, ShaderPipelineTargetState> mPipelineTargetStateMap;
};