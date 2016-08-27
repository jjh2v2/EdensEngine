#pragma once
#include "Render/Shader/Shader.h"

class ShaderPSO
{
public:
	ShaderPSO(ID3D12Device* device, Shader *shader, ShaderPipelineRenderState &renderState, ShaderPipelineTargetState &targetState, CD3DX12_ROOT_SIGNATURE_DESC &rootSignatureDesc);
	~ShaderPSO();

private:
	ID3D12RootSignature *mRootSignature;
	ID3D12PipelineState *mPipelineState;
};