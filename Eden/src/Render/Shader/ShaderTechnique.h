#pragma once
#include "Render/Shader/ShaderPSO.h"
#include "Core/Containers/DynamicArray.h"
#include <map>

class ShaderTechnique
{
public:
	ShaderTechnique(ID3D12Device *device, ShaderPipelineDefinition &pipelineDefinition);
	~ShaderTechnique();

	void AddAndCompilePermutation(ID3D12Device *device, const ShaderPipelinePermutation &permutation, ID3D12RootSignature *rootSignature);

private:
	Shader *mShader;
	std::map<ShaderPipelinePermutation, ShaderPSO*> mShaderPipelineMap;
	DynamicArray<ShaderPSO*> mShaderPipelines;
};