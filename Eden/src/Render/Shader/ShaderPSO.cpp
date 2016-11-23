#include "Render/Shader/ShaderPSO.h"

ShaderPSO::ShaderPSO(ID3D12Device* device, Shader *shader, ShaderPipelineRenderState &renderState, ShaderPipelineTargetState &targetState, ID3D12RootSignature *rootSignature)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.InputLayout = shader->GetInputLayoutDesc();
	pipelineStateDesc.pRootSignature = rootSignature;
	pipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(shader->GetVertexShader());
	pipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(shader->GetPixelShader());

	if (shader->GetHullShader())
	{
		pipelineStateDesc.HS = CD3DX12_SHADER_BYTECODE(shader->GetHullShader());
	}
	
	if (shader->GetDomainShader())
	{
		pipelineStateDesc.DS = CD3DX12_SHADER_BYTECODE(shader->GetDomainShader());
	}

	pipelineStateDesc.RasterizerState = renderState.RasterDesc;
	pipelineStateDesc.BlendState = renderState.BlendDesc;
	pipelineStateDesc.DepthStencilState = renderState.DepthStencilDesc;
	pipelineStateDesc.SampleMask = UINT_MAX;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; //TDA: need to be able to customize this

	pipelineStateDesc.NumRenderTargets = targetState.NumRenderTargets;
	for (UINT i = 0; i < targetState.NumRenderTargets; i++)
	{
		pipelineStateDesc.RTVFormats[i] = targetState.RenderTargetFormats[i];
	}

	pipelineStateDesc.DSVFormat = targetState.DepthStencilFormat;
	pipelineStateDesc.SampleDesc.Count = 1;

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&mPipelineState)));
}

ShaderPSO::~ShaderPSO()
{
	mPipelineState->Release();
	mPipelineState = NULL;
}