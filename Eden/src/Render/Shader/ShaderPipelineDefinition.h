#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/D3D12Helper.h"

enum ShaderDepthStencilStateType
{
	DepthStencil_Disabled = 0,
	DepthStencil_LessEqual,
	DepthStencil_GreaterEqual,
	DepthStencil_WriteLessEqual,
	DepthStencil_WriteGreaterEqual,
	DepthStencil_MaxDepthStencilStates
};

enum ShaderRasterStateType
{
	Raster_BackFaceCull = 0,
	Raster_BackFaceCullNoClip,
	Raster_FrontFaceCull,
	Raster_NoCull,
	Raster_WireFrame,
	Raster_MaxRasterStates
};

enum ShaderBlendStateType
{
	Blend_Disabled = 0,
	Blend_Additive,
	Blend_AlphaBlend,
	Blend_Multiply,
	Blend_Multiply2X,
	Blend_Subtract,
	Blend_MaxBlendStates
};

enum ShaderTargetStateType
{
	Target_GBuffer = 0
};


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

		for (uint32 i = 0; i < 8; i++)
		{
			RenderTargetFormats[i] = DXGI_FORMAT_UNKNOWN;
		}

		DepthStencilFormat = DXGI_FORMAT_UNKNOWN;
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

	ShaderPipelineRenderState(CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc, CD3DX12_RASTERIZER_DESC rasterDesc, CD3DX12_BLEND_DESC blendDesc)
	{
		DepthStencilDesc = depthStencilDesc;
		RasterDesc = rasterDesc;
		BlendDesc = blendDesc;
	}

	CD3DX12_DEPTH_STENCIL_DESC DepthStencilDesc;
	CD3DX12_RASTERIZER_DESC	RasterDesc;
	CD3DX12_BLEND_DESC BlendDesc;
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