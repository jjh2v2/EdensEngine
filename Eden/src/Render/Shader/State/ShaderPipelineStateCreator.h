#pragma once
#include "Render/Shader/ShaderPipelineDefinition.h"
#include <map>

class ShaderPipelineStateCreator
{
public:
	ShaderPipelineStateCreator();
	~ShaderPipelineStateCreator();

	static void BuildPipelineStates();
    static void DestroyPipelineStates();

	static ShaderPipelineRenderState &GetPipelineRenderState(ShaderRenderStateType stateType) { return mPipelineRenderStateMap[stateType]; }
	static ShaderPipelineTargetState &GetPipelineTargetState(ShaderTargetStateType stateType) { return mPipelineTargetStateMap[stateType]; }
    static ShaderPipelineInputLayout &GetPipelineInputLayout(ShaderInputLayoutType stateType) { return mPipelineInputLayoutMap[stateType]; }

private:
	static CD3DX12_DEPTH_STENCIL_DESC mDepthStencilDescs[DepthStencil_MaxDepthStencilStates];
	static CD3DX12_RASTERIZER_DESC mRasterDescs[Raster_MaxRasterStates];
	static CD3DX12_BLEND_DESC mBlendDescs[Blend_MaxBlendStates];

	static std::map<ShaderRenderStateType, ShaderPipelineRenderState> mPipelineRenderStateMap;
	static std::map<ShaderTargetStateType, ShaderPipelineTargetState> mPipelineTargetStateMap;
    static std::map<ShaderInputLayoutType, ShaderPipelineInputLayout> mPipelineInputLayoutMap;
};