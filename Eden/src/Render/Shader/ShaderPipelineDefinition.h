#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/D3D12Helper.h"
#include <string>

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
	Target_GBuffer = 0,
	Target_Standard_BackBuffer = 1,
	Target_Single_16 = 2
};

enum ShaderRenderStateType
{
	Render_Standard = 0
};


struct ShaderPipelineDefinition
{
	ShaderPipelineDefinition()
	{
		HasPixelShader = false;
		UsesTessellation = false;
		Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		Defines = NULL;
	}

	std::string ShaderOutputName;
	WCHAR VSFilePath[256];
	WCHAR PSFilePath[256];
	WCHAR HSFilePath[256];
	WCHAR DSFilePath[256];
	std::string VSEntry;
	std::string PSEntry;
	std::string HSEntry;
	std::string DSEntry;
	bool HasPixelShader;
	bool UsesTessellation;

	D3D12_PRIMITIVE_TOPOLOGY_TYPE Topology;
	D3D_SHADER_MACRO *Defines;
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
	ShaderPipelinePermutation(ShaderRenderStateType renderType, ShaderTargetStateType targetType)
	{
		RenderStateType = renderType;
		TargetStateType = targetType;
	}

	bool operator < (const ShaderPipelinePermutation &permutation) const
	{
		return RenderStateType != permutation.RenderStateType ?
			RenderStateType < permutation.RenderStateType :
			TargetStateType < permutation.TargetStateType;
	}

	bool operator == (const ShaderPipelinePermutation &permutation) const
	{
		return (RenderStateType == permutation.RenderStateType) && (TargetStateType == permutation.TargetStateType);
	}


	ShaderRenderStateType RenderStateType;
	ShaderTargetStateType TargetStateType;
};