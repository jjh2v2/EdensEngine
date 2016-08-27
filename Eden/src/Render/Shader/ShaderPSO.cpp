#include "Render/Shader/ShaderPSO.h"

ShaderPSO::ShaderPSO(ID3D12Device* device, Shader *shader, ShaderPipelineRenderState &renderState, ShaderPipelineTargetState &targetState, CD3DX12_ROOT_SIGNATURE_DESC &rootSignatureDesc)
{
	ID3DBlob* rootSignature;
	ID3DBlob* rootSignatureError;
	Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignature, &rootSignatureError));
	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, rootSignature->GetBufferPointer(), rootSignature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.InputLayout = shader->GetInputLayoutDesc();
	pipelineStateDesc.pRootSignature = mRootSignature;
	pipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(shader->GetVertexShader());
	pipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(shader->GetPixelShader());
	pipelineStateDesc.HS = CD3DX12_SHADER_BYTECODE(shader->GetHullShader());
	pipelineStateDesc.DS = CD3DX12_SHADER_BYTECODE(shader->GetDomainShader());

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