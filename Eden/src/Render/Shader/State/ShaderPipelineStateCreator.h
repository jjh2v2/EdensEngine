#pragma once
#include "Render/Shader/ShaderPipelineDefinition.h"
#include <map>

class ShaderPipelineStateCreator
{
public:
	ShaderPipelineStateCreator();
	~ShaderPipelineStateCreator();

	static void BuildPipelineStates();

	static ShaderPipelineRenderState &GetPipelineRenderState(ShaderRenderStateType stateType) { return mPipelineRenderStateMap[stateType]; }
	static ShaderPipelineTargetState &GetPipelineTargetState(ShaderTargetStateType stateType) { return mPipelineTargetStateMap[stateType]; }

private:
	static CD3DX12_DEPTH_STENCIL_DESC mDepthStencilDescs[DepthStencil_MaxDepthStencilStates];
	static CD3DX12_RASTERIZER_DESC mRasterDescs[Raster_MaxRasterStates];
	static CD3DX12_BLEND_DESC mBlendDescs[Blend_MaxBlendStates];

	static std::map<ShaderRenderStateType, ShaderPipelineRenderState> mPipelineRenderStateMap;
	static std::map<ShaderTargetStateType, ShaderPipelineTargetState> mPipelineTargetStateMap;
};