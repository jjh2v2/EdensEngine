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
    Target_None = 0,
	Target_GBuffer = 1,
	Target_Standard_BackBuffer = 2,
	Target_Standard_BackBuffer_NoDepth = 3,
	Target_Single_16 = 4,
	Target_Single_16_NoDepth = 5,
    Target_Single_32_NoDepth = 6,
    Target_Depth_Stencil_Only_32_Sample_4 = 7,
    Target_Single_R32_NoDepth = 8,
    Target_Depth_Only = 9,
};

enum ShaderRenderStateType
{
    Render_None = 0,
	Render_Standard = 1,
	Render_Standard_NoDepth = 2,
    Render_ShadowMap = 3,
    Render_Sky = 4,
    Render_Water = 5,
    Render_Water_Depth = 6,
};

enum ShaderInputLayoutType
{
    InputLayout_None = 0,
    InputLayout_Standard = 1
};

struct ShaderPipelineDefinition
{
	ShaderPipelineDefinition()
	{
        IsRenderShader = false;
		HasPixelShader = false;
        HasTessellation = false;
		Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		Defines = NULL;
	}

	std::string ShaderOutputName;

	WCHAR VSFilePath[256];
	WCHAR PSFilePath[256];
	WCHAR HSFilePath[256];
	WCHAR DSFilePath[256];
    WCHAR CSFilePath[256];

	std::string VSEntry;
	std::string PSEntry;
	std::string HSEntry;
	std::string DSEntry;
    std::string CSEntry;

    bool IsRenderShader;
	bool HasPixelShader;
	bool HasTessellation;

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
        SampleCount = 1;
        SampleQuality = 0;
	}

	UINT NumRenderTargets;
	DXGI_FORMAT RenderTargetFormats[8];
	DXGI_FORMAT DepthStencilFormat;
    UINT SampleCount;
    UINT SampleQuality;
};

struct ShaderPipelineRenderState
{
	ShaderPipelineRenderState()
	{
		DepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC{};
		RasterDesc = CD3DX12_RASTERIZER_DESC{};
		BlendDesc = CD3DX12_BLEND_DESC{};
		TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}

	ShaderPipelineRenderState(CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc, CD3DX12_RASTERIZER_DESC rasterDesc, CD3DX12_BLEND_DESC blendDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
	{
		DepthStencilDesc = depthStencilDesc;
		RasterDesc = rasterDesc;
		BlendDesc = blendDesc;
		TopologyType = topologyType;
	}

	CD3DX12_DEPTH_STENCIL_DESC DepthStencilDesc;
	CD3DX12_RASTERIZER_DESC	RasterDesc;
	CD3DX12_BLEND_DESC BlendDesc;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType;
};

struct ShaderPipelinePermutation
{
    ShaderPipelinePermutation()
    {
        RenderStateType = Render_None;
        TargetStateType = Target_None;
        InputLayoutType = InputLayout_None;
    }

	ShaderPipelinePermutation(ShaderRenderStateType renderType, ShaderTargetStateType targetType, ShaderInputLayoutType layoutType)
	{
		RenderStateType = renderType;
		TargetStateType = targetType;
        InputLayoutType = layoutType;
	}

	bool operator < (const ShaderPipelinePermutation &permutation) const
	{
        if (RenderStateType != permutation.RenderStateType)
        {
            return RenderStateType < permutation.RenderStateType;
        }
        else if (TargetStateType != permutation.TargetStateType)
        {
            return TargetStateType < permutation.TargetStateType;
        }
        else
        {
            return InputLayoutType < permutation.InputLayoutType;
        }
	}

	bool operator == (const ShaderPipelinePermutation &permutation) const
	{
		return (RenderStateType == permutation.RenderStateType) && (TargetStateType == permutation.TargetStateType) && (InputLayoutType == permutation.InputLayoutType);
	}


	ShaderRenderStateType RenderStateType;
	ShaderTargetStateType TargetStateType;
    ShaderInputLayoutType InputLayoutType;
};

struct ShaderPipelineInputLayout
{
    D3D12_INPUT_ELEMENT_DESC *InputElementDescs;
    D3D12_INPUT_LAYOUT_DESC   InputLayoutDesc;
};