#include "Render/Shader/ShaderTechnique.h"
#include "Render/Shader/State/ShaderPipelineStateCreator.h"

ShaderTechnique::ShaderTechnique(ShaderPipelineDefinition pipelineDefinition)
{
	mBaseShaderPipeline = pipelineDefinition;
}

ShaderTechnique::~ShaderTechnique()
{

}

void ShaderTechnique::AddAndCompilePermutation(ID3D12Device *device, const ShaderPipelinePermutation &permutation)
{
	D3D_SHADER_MACRO defines[1];

	Shader *newShader = new Shader(device, mBaseShaderPipeline, ShaderPipelineStateCreator::GetPipelineRenderState(permutation.RenderStateType),
		ShaderPipelineStateCreator::GetPipelineTargetState(permutation.TargetStateType), defines);

	mShaderPipelineMap.insert(std::pair<ShaderPipelinePermutation, Shader*>(permutation, newShader));
}