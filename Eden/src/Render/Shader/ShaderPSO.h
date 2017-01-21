#pragma once
#include "Render/Shader/Shader.h"

class ShaderPSO
{
public:
	ShaderPSO(ID3D12Device* device, Shader *shader, ShaderPipelineRenderState &renderState, ShaderPipelineTargetState &targetState, ID3D12RootSignature *rootSignature);
	~ShaderPSO();

	ID3D12PipelineState *GetPipelineState() { return mPipelineState; }
	ID3D12RootSignature *GetRootSignature() { return mRootSignature; }

private:
	ID3D12PipelineState *mPipelineState;
	ID3D12RootSignature *mRootSignature;
};