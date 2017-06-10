#include "Render/Shader/ShaderTechnique.h"
#include "Render/Shader/State/ShaderPipelineStateCreator.h"

ShaderTechnique::ShaderTechnique(ID3D12Device *device, ShaderPipelineDefinition &pipelineDefinition)
{
	mShader = new Shader(device, pipelineDefinition);
}

ShaderTechnique::~ShaderTechnique()
{
	delete mShader;

	for (uint32 i = 0; i < mShaderPipelines.CurrentSize(); i++)
	{
		delete mShaderPipelines[i];
	}
	mShaderPipelines.Clear();
	mShaderPipelineMap.clear();
}

void ShaderTechnique::AddAndCompilePermutation(ID3D12Device *device, const ShaderPipelinePermutation &permutation, ID3D12RootSignature *rootSignature)
{
	ShaderPipelineRenderState &renderState = ShaderPipelineStateCreator::GetPipelineRenderState(permutation.RenderStateType);
	ShaderPipelineTargetState &targetState = ShaderPipelineStateCreator::GetPipelineTargetState(permutation.TargetStateType);

	ShaderPSO *newShaderPSO = new ShaderPSO(device, mShader, renderState, targetState, rootSignature);
	mShaderPipelines.Add(newShaderPSO);

	mShaderPipelineMap.insert(std::pair<ShaderPipelinePermutation, ShaderPSO*>(permutation, newShaderPSO));
}

ShaderPSO *ShaderTechnique::GetShader(ShaderPipelinePermutation permutation)
{
	return mShaderPipelineMap[permutation];
}

bool ShaderTechnique::HasShader(ShaderPipelinePermutation permutation)
{
	return mShaderPipelineMap.find(permutation) != mShaderPipelineMap.end();
}