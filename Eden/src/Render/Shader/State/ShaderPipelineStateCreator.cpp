#include "Render/Shader/State/ShaderPipelineStateCreator.h"

ShaderPipelineStateCreator::ShaderPipelineStateCreator()
{
	mDepthStencilDescs[DepthStencil_Disabled] = CD3DX12_DEPTH_STENCIL_DESC{};
	mDepthStencilDescs[DepthStencil_Disabled].DepthEnable = false;
	mDepthStencilDescs[DepthStencil_Disabled].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mDepthStencilDescs[DepthStencil_Disabled].DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	mDepthStencilDescs[DepthStencil_EnabledLessEqual] = CD3DX12_DEPTH_STENCIL_DESC{};
	mDepthStencilDescs[DepthStencil_EnabledLessEqual].DepthEnable = true;
	mDepthStencilDescs[DepthStencil_EnabledLessEqual].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mDepthStencilDescs[DepthStencil_EnabledLessEqual].DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	mDepthStencilDescs[DepthStencil_EnabledGreaterEqual] = CD3DX12_DEPTH_STENCIL_DESC{};
	mDepthStencilDescs[DepthStencil_EnabledGreaterEqual].DepthEnable = true;
	mDepthStencilDescs[DepthStencil_EnabledGreaterEqual].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mDepthStencilDescs[DepthStencil_EnabledGreaterEqual].DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

	mDepthStencilDescs[DepthStencil_EnabledWriteLessEqual] = CD3DX12_DEPTH_STENCIL_DESC{};
	mDepthStencilDescs[DepthStencil_EnabledWriteLessEqual].DepthEnable = true;
	mDepthStencilDescs[DepthStencil_EnabledWriteLessEqual].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	mDepthStencilDescs[DepthStencil_EnabledWriteLessEqual].DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	mDepthStencilDescs[DepthStencil_EnabledWriteGreaterEqual] = CD3DX12_DEPTH_STENCIL_DESC{};
	mDepthStencilDescs[DepthStencil_EnabledWriteGreaterEqual].DepthEnable = true;
	mDepthStencilDescs[DepthStencil_EnabledWriteGreaterEqual].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	mDepthStencilDescs[DepthStencil_EnabledWriteGreaterEqual].DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

	mRasterDescs[Raster_BackFaceCull] = CD3DX12_RASTERIZER_DESC{};
	mRasterDescs[Raster_BackFaceCull].CullMode = D3D12_CULL_MODE_BACK;
	mRasterDescs[Raster_BackFaceCull].FillMode = D3D12_FILL_MODE_SOLID;
	mRasterDescs[Raster_BackFaceCull].MultisampleEnable = true;
	mRasterDescs[Raster_BackFaceCull].DepthClipEnable = true;

	mRasterDescs[Raster_BackFaceCullNoClip] = CD3DX12_RASTERIZER_DESC{};
	mRasterDescs[Raster_BackFaceCullNoClip].CullMode = D3D12_CULL_MODE_BACK;
	mRasterDescs[Raster_BackFaceCullNoClip].FillMode = D3D12_FILL_MODE_SOLID;
	mRasterDescs[Raster_BackFaceCullNoClip].MultisampleEnable = true;
	mRasterDescs[Raster_BackFaceCullNoClip].DepthClipEnable = false;

	mRasterDescs[Raster_FrontFaceCull] = CD3DX12_RASTERIZER_DESC{};
	mRasterDescs[Raster_FrontFaceCull].CullMode = D3D12_CULL_MODE_FRONT;
	mRasterDescs[Raster_FrontFaceCull].FillMode = D3D12_FILL_MODE_SOLID;
	mRasterDescs[Raster_FrontFaceCull].MultisampleEnable = true;
	mRasterDescs[Raster_FrontFaceCull].DepthClipEnable = true;

	mRasterDescs[Raster_NoCull] = CD3DX12_RASTERIZER_DESC{};
	mRasterDescs[Raster_NoCull].CullMode = D3D12_CULL_MODE_NONE;
	mRasterDescs[Raster_NoCull].FillMode = D3D12_FILL_MODE_SOLID;
	mRasterDescs[Raster_NoCull].MultisampleEnable = true;
	mRasterDescs[Raster_NoCull].DepthClipEnable = true;

	mRasterDescs[Raster_WireFrame] = CD3DX12_RASTERIZER_DESC{};
	mRasterDescs[Raster_WireFrame].CullMode = D3D12_CULL_MODE_NONE;
	mRasterDescs[Raster_WireFrame].FillMode = D3D12_FILL_MODE_WIREFRAME;
	mRasterDescs[Raster_WireFrame].MultisampleEnable = true;
	mRasterDescs[Raster_WireFrame].DepthClipEnable = true;

	mBlendDescs[Blend_Disabled] = CD3DX12_BLEND_DESC{};
	mBlendDescs[Blend_Disabled].RenderTarget[0].BlendEnable = false;
	mBlendDescs[Blend_Disabled].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Disabled].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Disabled].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	mBlendDescs[Blend_Disabled].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Disabled].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	mBlendDescs[Blend_Disabled].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Disabled].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendDescs[Blend_Additive] = CD3DX12_BLEND_DESC{};
	mBlendDescs[Blend_Additive].RenderTarget[0].BlendEnable = true;
	mBlendDescs[Blend_Additive].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Additive].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Additive].RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Additive].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Additive].RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Additive].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Additive].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendDescs[Blend_AlphaBlend] = CD3DX12_BLEND_DESC{};
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].BlendEnable = true;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendDescs[Blend_Multiply] = CD3DX12_BLEND_DESC{};
	mBlendDescs[Blend_Multiply].RenderTarget[0].BlendEnable = true;
	mBlendDescs[Blend_Multiply].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Multiply].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Multiply].RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
	mBlendDescs[Blend_Multiply].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Multiply].RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
	mBlendDescs[Blend_Multiply].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Multiply].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendDescs[Blend_Multiply2X] = CD3DX12_BLEND_DESC{};
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].BlendEnable = true;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].SrcBlend = D3D12_BLEND_DEST_COLOR;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendDescs[Blend_Subtract] = CD3DX12_BLEND_DESC{};
	mBlendDescs[Blend_Subtract].RenderTarget[0].BlendEnable = true;
	mBlendDescs[Blend_Subtract].RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
	mBlendDescs[Blend_Subtract].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Subtract].RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Subtract].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Subtract].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	mBlendDescs[Blend_Subtract].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Subtract].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
}

ShaderPipelineStateCreator::~ShaderPipelineStateCreator()
{

}