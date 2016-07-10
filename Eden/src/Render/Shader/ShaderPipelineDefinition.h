#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/D3D12Helper.h"

struct ShaderPipelineDefinition
{
	ShaderPipelineDefinition()
	{
		Defines = NULL;
		VSFilePath = NULL;
		PSFilePath = NULL;
		HSFilePath = NULL;
		DSFilePath = NULL;
		VSEntry = NULL;
		PSEntry = NULL;
		HSEntry = NULL;
		DSEntry = NULL;
		HasPixelShader = false;
		UsesTessellation = false;

		DepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC{};
		RasterDesc = CD3DX12_RASTERIZER_DESC{};
		BlendDesc = CD3DX12_BLEND_DESC{};
		RootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC{};

		Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		NumRenderTargets = 1;
	}

	std::string ShaderOutputName;
	D3D_SHADER_MACRO *Defines;
	WCHAR* VSFilePath;
	WCHAR* PSFilePath;
	WCHAR* HSFilePath;
	WCHAR* DSFilePath;
	LPCSTR VSEntry;
	LPCSTR PSEntry;
	LPCSTR HSEntry;
	LPCSTR DSEntry;
	bool HasPixelShader;
	bool UsesTessellation;

	CD3DX12_DEPTH_STENCIL_DESC DepthStencilDesc;
	CD3DX12_RASTERIZER_DESC	RasterDesc;
	CD3DX12_BLEND_DESC BlendDesc;

	D3D12_PRIMITIVE_TOPOLOGY_TYPE Topology;
	UINT NumRenderTargets;
	DXGI_FORMAT RenderTargetFormats[8];
	DXGI_FORMAT DepthStencilFormat;

	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
};