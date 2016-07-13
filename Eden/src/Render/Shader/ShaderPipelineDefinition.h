#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/D3D12Helper.h"

struct ShaderPipelineDefinition
{
	ShaderPipelineDefinition()
	{
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
		Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		RootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC{};
	}

	std::string ShaderOutputName;
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

	D3D12_PRIMITIVE_TOPOLOGY_TYPE Topology;
	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
};

struct ShaderPipelineTargetState
{
	ShaderPipelineTargetState()
	{
		NumRenderTargets = 0;
	}

	UINT NumRenderTargets;
	DXGI_FORMAT RenderTargetFormats[8];
	DXGI_FORMAT DepthStencilFormat;
};

struct ShaderPipelineRenderState
{
	ShaderPipelineRenderState()
	{
		DepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC{};
		RasterDesc = CD3DX12_RASTERIZER_DESC{};
		BlendDesc = CD3DX12_BLEND_DESC{};
	}

	CD3DX12_DEPTH_STENCIL_DESC DepthStencilDesc;
	CD3DX12_RASTERIZER_DESC	RasterDesc;
	CD3DX12_BLEND_DESC BlendDesc;
};

enum ShaderDepthStencilState
{
	Disabled = 0,
	EnabledLessEqual,
	EnabledGreaterEqual,
	EnabledWriteLessEqual,
	EnabledWriteGreaterEqual,
	MaxDepthStencilStates
};

enum ShaderRasterState
{
	BackFaceCull = 0,
	BackFaceCullNoClip,
	FrontFaceCull,
	NoCull,
	WireFrame,
	MaxRasterStates
};

enum ShaderBlendState
{
	Disabled = 0,
	Additive,
	AlphaBlend,
	Multiply,
	Multiply2X,
	Subtract,
	MaxBlendStates
};

struct ShaderPipelinePermutation
{
	ShaderPipelinePermutation()
	{
		RenderStateID = 0;
		TargetStateID = 0;
		ShaderDefines = 0;
	}

	bool operator < (const ShaderPipelinePermutation &permutation) const
	{
		return RenderStateID != permutation.RenderStateID ?
			RenderStateID < permutation.RenderStateID :
			TargetStateID != permutation.TargetStateID ?
			TargetStateID < permutation.TargetStateID :
			ShaderDefines < permutation.ShaderDefines;
	}

	bool operator == (const ShaderPipelinePermutation &permutation) const
	{
		return (RenderStateID == permutation.RenderStateID) && (TargetStateID == permutation.TargetStateID) && (ShaderDefines == permutation.ShaderDefines);
	}


	//we can swap these out for render states
	uint32 RenderStateID;
	uint32 TargetStateID;
	uint64 ShaderDefines;
};