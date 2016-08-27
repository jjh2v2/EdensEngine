#include "Render/Shader/ShaderTechnique.h"
#include "Render/Shader/State/ShaderPipelineStateCreator.h"

ShaderTechnique::ShaderTechnique(ID3D12Device *device, ShaderPipelineDefinition &pipelineDefinition)
{
	mShader = new Shader(device, pipelineDefinition);
}

ShaderTechnique::~ShaderTechnique()
{

}

void ShaderTechnique::AddAndCompilePermutation(ID3D12Device *device, const ShaderPipelinePermutation &permutation)
{
	ShaderPSO *newShaderPSO = new ShaderPSO(device, mShader, ShaderPipelineStateCreator::GetPipelineRenderState(permutation.RenderStateType),
		ShaderPipelineStateCreator::GetPipelineTargetState(permutation.TargetStateType));

	mShaderPipelineMap.insert(std::pair<ShaderPipelinePermutation, ShaderPSO*>(permutation, newShaderPSO));
}