#pragma once
#include "Render/Shader/Shader.h"
#include "Core/Containers/DynamicArray.h"
#include <map>

class ShaderTechnique
{
public:
	ShaderTechnique(ShaderPipelineDefinition pipelineDefinition);
	~ShaderTechnique();

	void AddAndCompilePermutation(ID3D12Device *device, const ShaderPipelinePermutation &permutation);

private:
	std::map<ShaderPipelinePermutation, Shader*> mShaderPipelineMap;
	ShaderPipelineDefinition mBaseShaderPipeline;
};