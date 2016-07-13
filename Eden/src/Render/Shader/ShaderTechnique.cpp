#include "Render/Shader/ShaderTechnique.h"

ShaderTechnique::ShaderTechnique(ShaderPipelineDefinition pipelineDefinition)
{
	mBaseShaderPipeline = pipelineDefinition;
}

ShaderTechnique::~ShaderTechnique()
{

}

void ShaderTechnique::AddAndCompilePermutation(ID3D12Device *device, const ShaderPipelinePermutation &permutation)
{
	Shader *newShader = new Shader(device, mBaseShaderPipeline, )
}