#pragma once
#include "Render/Shader/Shader.h"

class ShaderPSO
{
public:
	ShaderPSO(ID3D12Device* device, Shader *shader, ShaderPipelineRenderState &renderState, ShaderPipelineTargetState &targetState, ShaderPipelineInputLayout &layout, ID3D12RootSignature *rootSignature);
    ShaderPSO(ID3D12Device* device, Shader *shader, ID3D12RootSignature *rootSignature);
    ~ShaderPSO();

	ID3D12PipelineState *GetPipelineState() { return mPipelineState; }
	ID3D12RootSignature *GetRootSignature() { return mRootSignature; }

private:
    bool mIsComputeShader;
	ID3D12PipelineState *mPipelineState;
	ID3D12RootSignature *mRootSignature;
};