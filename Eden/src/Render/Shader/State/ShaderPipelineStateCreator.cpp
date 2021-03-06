#include "Render/Shader/State/ShaderPipelineStateCreator.h"
#include "Render/Shader/ShaderPipelineDefinition.h"

CD3DX12_DEPTH_STENCIL_DESC ShaderPipelineStateCreator::mDepthStencilDescs[];
CD3DX12_RASTERIZER_DESC ShaderPipelineStateCreator::mRasterDescs[];
CD3DX12_BLEND_DESC ShaderPipelineStateCreator::mBlendDescs[];

std::map<ShaderRenderStateType, ShaderPipelineRenderState> ShaderPipelineStateCreator::mPipelineRenderStateMap;
std::map<ShaderTargetStateType, ShaderPipelineTargetState> ShaderPipelineStateCreator::mPipelineTargetStateMap;
std::map<ShaderInputLayoutType, ShaderPipelineInputLayout> ShaderPipelineStateCreator::mPipelineInputLayoutMap;

ShaderPipelineStateCreator::ShaderPipelineStateCreator()
{
	
}

ShaderPipelineStateCreator::~ShaderPipelineStateCreator()
{

}

void ShaderPipelineStateCreator::DestroyPipelineStates()
{
    mPipelineRenderStateMap.clear();
    mPipelineTargetStateMap.clear();

    for (auto iter = mPipelineInputLayoutMap.begin(); iter != mPipelineInputLayoutMap.end(); iter++)
    {
        if (iter->second.InputElementDescs)
        {
            delete [] iter->second.InputElementDescs;
        }
    }
}

void ShaderPipelineStateCreator::BuildPipelineStates()
{
	mDepthStencilDescs[DepthStencil_Disabled] = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	mDepthStencilDescs[DepthStencil_Disabled].DepthEnable = false;
	mDepthStencilDescs[DepthStencil_Disabled].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mDepthStencilDescs[DepthStencil_Disabled].DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	mDepthStencilDescs[DepthStencil_LessEqual] = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	mDepthStencilDescs[DepthStencil_LessEqual].DepthEnable = true;
	mDepthStencilDescs[DepthStencil_LessEqual].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mDepthStencilDescs[DepthStencil_LessEqual].DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	mDepthStencilDescs[DepthStencil_GreaterEqual] = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	mDepthStencilDescs[DepthStencil_GreaterEqual].DepthEnable = true;
	mDepthStencilDescs[DepthStencil_GreaterEqual].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mDepthStencilDescs[DepthStencil_GreaterEqual].DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

	mDepthStencilDescs[DepthStencil_WriteLessEqual] = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	mDepthStencilDescs[DepthStencil_WriteLessEqual].DepthEnable = true;
	mDepthStencilDescs[DepthStencil_WriteLessEqual].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	mDepthStencilDescs[DepthStencil_WriteLessEqual].DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	mDepthStencilDescs[DepthStencil_WriteGreaterEqual] = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	mDepthStencilDescs[DepthStencil_WriteGreaterEqual].DepthEnable = true;
	mDepthStencilDescs[DepthStencil_WriteGreaterEqual].DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	mDepthStencilDescs[DepthStencil_WriteGreaterEqual].DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

	mRasterDescs[Raster_BackFaceCull] = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	mRasterDescs[Raster_BackFaceCull].CullMode = D3D12_CULL_MODE_BACK;
	mRasterDescs[Raster_BackFaceCull].FillMode = D3D12_FILL_MODE_SOLID;
	mRasterDescs[Raster_BackFaceCull].MultisampleEnable = true;
	mRasterDescs[Raster_BackFaceCull].DepthClipEnable = true;

	mRasterDescs[Raster_BackFaceCullNoClip] = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	mRasterDescs[Raster_BackFaceCullNoClip].CullMode = D3D12_CULL_MODE_BACK;
	mRasterDescs[Raster_BackFaceCullNoClip].FillMode = D3D12_FILL_MODE_SOLID;
	mRasterDescs[Raster_BackFaceCullNoClip].MultisampleEnable = true;
	mRasterDescs[Raster_BackFaceCullNoClip].DepthClipEnable = false;

	mRasterDescs[Raster_FrontFaceCull] = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	mRasterDescs[Raster_FrontFaceCull].CullMode = D3D12_CULL_MODE_FRONT;
	mRasterDescs[Raster_FrontFaceCull].FillMode = D3D12_FILL_MODE_SOLID;
	mRasterDescs[Raster_FrontFaceCull].MultisampleEnable = true;
	mRasterDescs[Raster_FrontFaceCull].DepthClipEnable = true;

	mRasterDescs[Raster_NoCull] = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	mRasterDescs[Raster_NoCull].CullMode = D3D12_CULL_MODE_NONE;
	mRasterDescs[Raster_NoCull].FillMode = D3D12_FILL_MODE_SOLID;
	mRasterDescs[Raster_NoCull].MultisampleEnable = true;
	mRasterDescs[Raster_NoCull].DepthClipEnable = true;

	mRasterDescs[Raster_WireFrame] = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	mRasterDescs[Raster_WireFrame].CullMode = D3D12_CULL_MODE_NONE;
	mRasterDescs[Raster_WireFrame].FillMode = D3D12_FILL_MODE_WIREFRAME;
	mRasterDescs[Raster_WireFrame].MultisampleEnable = true;
	mRasterDescs[Raster_WireFrame].DepthClipEnable = true;

	mBlendDescs[Blend_Disabled] = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	mBlendDescs[Blend_Disabled].RenderTarget[0].BlendEnable = false;
	mBlendDescs[Blend_Disabled].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Disabled].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Disabled].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	mBlendDescs[Blend_Disabled].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Disabled].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	mBlendDescs[Blend_Disabled].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Disabled].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendDescs[Blend_Additive] = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	mBlendDescs[Blend_Additive].RenderTarget[0].BlendEnable = true;
	mBlendDescs[Blend_Additive].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Additive].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Additive].RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Additive].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Additive].RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Additive].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Additive].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendDescs[Blend_AlphaBlend] = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].BlendEnable = true;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_AlphaBlend].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendDescs[Blend_Multiply] = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	mBlendDescs[Blend_Multiply].RenderTarget[0].BlendEnable = true;
	mBlendDescs[Blend_Multiply].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Multiply].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Multiply].RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
	mBlendDescs[Blend_Multiply].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Multiply].RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
	mBlendDescs[Blend_Multiply].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Multiply].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendDescs[Blend_Multiply2X] = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].BlendEnable = true;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].SrcBlend = D3D12_BLEND_DEST_COLOR;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Multiply2X].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendDescs[Blend_Subtract] = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	mBlendDescs[Blend_Subtract].RenderTarget[0].BlendEnable = true;
	mBlendDescs[Blend_Subtract].RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
	mBlendDescs[Blend_Subtract].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendDescs[Blend_Subtract].RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Subtract].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Subtract].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	mBlendDescs[Blend_Subtract].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendDescs[Blend_Subtract].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mPipelineRenderStateMap.insert(std::pair<ShaderRenderStateType, ShaderPipelineRenderState>(Render_Standard,
		ShaderPipelineRenderState(mDepthStencilDescs[DepthStencil_WriteGreaterEqual], mRasterDescs[Raster_BackFaceCull], mBlendDescs[Blend_Disabled], D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)));

    mPipelineRenderStateMap.insert(std::pair<ShaderRenderStateType, ShaderPipelineRenderState>(Render_Water,
        ShaderPipelineRenderState(mDepthStencilDescs[DepthStencil_GreaterEqual], mRasterDescs[Raster_BackFaceCull], mBlendDescs[Blend_AlphaBlend], D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH)));

    mPipelineRenderStateMap.insert(std::pair<ShaderRenderStateType, ShaderPipelineRenderState>(Render_Water_Depth,
        ShaderPipelineRenderState(mDepthStencilDescs[DepthStencil_WriteGreaterEqual], mRasterDescs[Raster_BackFaceCull], mBlendDescs[Blend_Disabled], D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH)));

	mPipelineRenderStateMap.insert(std::pair<ShaderRenderStateType, ShaderPipelineRenderState>(Render_Standard_NoDepth,
		ShaderPipelineRenderState(mDepthStencilDescs[DepthStencil_Disabled], mRasterDescs[Raster_BackFaceCull], mBlendDescs[Blend_Disabled], D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)));

    mPipelineRenderStateMap.insert(std::pair<ShaderRenderStateType, ShaderPipelineRenderState>(Render_ShadowMap,
        ShaderPipelineRenderState(mDepthStencilDescs[DepthStencil_WriteLessEqual], mRasterDescs[Raster_BackFaceCullNoClip], mBlendDescs[Blend_Disabled], D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)));

    mPipelineRenderStateMap.insert(std::pair<ShaderRenderStateType, ShaderPipelineRenderState>(Render_Sky,
        ShaderPipelineRenderState(mDepthStencilDescs[DepthStencil_Disabled], mRasterDescs[Raster_FrontFaceCull], mBlendDescs[Blend_Disabled], D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)));

	ShaderPipelineTargetState gbufferTarget;
	gbufferTarget.NumRenderTargets = 3;
	gbufferTarget.RenderTargetFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;	//albedo
	gbufferTarget.RenderTargetFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;  //normals
	gbufferTarget.RenderTargetFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT;  //material
	gbufferTarget.DepthStencilFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

	mPipelineTargetStateMap.insert(std::pair<ShaderTargetStateType, ShaderPipelineTargetState>(Target_GBuffer, gbufferTarget));

	ShaderPipelineTargetState standardSingleTarget;
	standardSingleTarget.NumRenderTargets = 1;
	standardSingleTarget.RenderTargetFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	standardSingleTarget.DepthStencilFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

	mPipelineTargetStateMap.insert(std::pair<ShaderTargetStateType, ShaderPipelineTargetState>(Target_Standard_BackBuffer, standardSingleTarget));

	ShaderPipelineTargetState standardSingleTargetNoDepth;
	standardSingleTargetNoDepth.NumRenderTargets = 1;
	standardSingleTargetNoDepth.RenderTargetFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	standardSingleTargetNoDepth.DepthStencilFormat = DXGI_FORMAT_UNKNOWN;

	mPipelineTargetStateMap.insert(std::pair<ShaderTargetStateType, ShaderPipelineTargetState>(Target_Standard_BackBuffer_NoDepth, standardSingleTargetNoDepth));

	ShaderPipelineTargetState single16Target;
	single16Target.NumRenderTargets = 1;
	single16Target.RenderTargetFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	single16Target.DepthStencilFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

	mPipelineTargetStateMap.insert(std::pair<ShaderTargetStateType, ShaderPipelineTargetState>(Target_Single_16, single16Target));

    ShaderPipelineTargetState singleDepthOnly;
    singleDepthOnly.NumRenderTargets = 0;
    singleDepthOnly.DepthStencilFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

    mPipelineTargetStateMap.insert(std::pair<ShaderTargetStateType, ShaderPipelineTargetState>(Target_Depth_Only, singleDepthOnly));

	ShaderPipelineTargetState single16NoDepthTarget;
	single16NoDepthTarget.NumRenderTargets = 1;
	single16NoDepthTarget.RenderTargetFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	single16NoDepthTarget.DepthStencilFormat = DXGI_FORMAT_UNKNOWN;

	mPipelineTargetStateMap.insert(std::pair<ShaderTargetStateType, ShaderPipelineTargetState>(Target_Single_16_NoDepth, single16NoDepthTarget));

    ShaderPipelineTargetState single32NoDepthTarget;
    single32NoDepthTarget.NumRenderTargets = 1;
    single32NoDepthTarget.RenderTargetFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    single32NoDepthTarget.DepthStencilFormat = DXGI_FORMAT_UNKNOWN;

    mPipelineTargetStateMap.insert(std::pair<ShaderTargetStateType, ShaderPipelineTargetState>(Target_Single_32_NoDepth, single32NoDepthTarget));

    ShaderPipelineTargetState depthStencilOnly32Target;
    depthStencilOnly32Target.NumRenderTargets = 0;
    depthStencilOnly32Target.DepthStencilFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    depthStencilOnly32Target.SampleCount = 4;
    depthStencilOnly32Target.SampleQuality = 0;

    mPipelineTargetStateMap.insert(std::pair<ShaderTargetStateType, ShaderPipelineTargetState>(Target_Depth_Stencil_Only_32_Sample_4, depthStencilOnly32Target));

    ShaderPipelineTargetState singleR32NoDepthTarget;
    singleR32NoDepthTarget.NumRenderTargets = 1;
    singleR32NoDepthTarget.RenderTargetFormats[0] = DXGI_FORMAT_R32_FLOAT;
    singleR32NoDepthTarget.DepthStencilFormat = DXGI_FORMAT_UNKNOWN;

    mPipelineTargetStateMap.insert(std::pair<ShaderTargetStateType, ShaderPipelineTargetState>(Target_Single_R32_NoDepth, singleR32NoDepthTarget));

    ShaderPipelineInputLayout standardInputLayout;
    standardInputLayout.InputElementDescs = new D3D12_INPUT_ELEMENT_DESC[6];
    standardInputLayout.InputElementDescs[0].SemanticName = "POSITION";
    standardInputLayout.InputElementDescs[0].SemanticIndex = 0;
    standardInputLayout.InputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    standardInputLayout.InputElementDescs[0].InputSlot = 0;
    standardInputLayout.InputElementDescs[0].AlignedByteOffset = 0;
    standardInputLayout.InputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    standardInputLayout.InputElementDescs[0].InstanceDataStepRate = 0;

    standardInputLayout.InputElementDescs[1].SemanticName = "TEXCOORD";
    standardInputLayout.InputElementDescs[1].SemanticIndex = 0;
    standardInputLayout.InputElementDescs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    standardInputLayout.InputElementDescs[1].InputSlot = 0;
    standardInputLayout.InputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    standardInputLayout.InputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    standardInputLayout.InputElementDescs[1].InstanceDataStepRate = 0;

    standardInputLayout.InputElementDescs[2].SemanticName = "NORMAL";
    standardInputLayout.InputElementDescs[2].SemanticIndex = 0;
    standardInputLayout.InputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    standardInputLayout.InputElementDescs[2].InputSlot = 0;
    standardInputLayout.InputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    standardInputLayout.InputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    standardInputLayout.InputElementDescs[2].InstanceDataStepRate = 0;

    standardInputLayout.InputElementDescs[3].SemanticName = "TANGENT";
    standardInputLayout.InputElementDescs[3].SemanticIndex = 0;
    standardInputLayout.InputElementDescs[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    standardInputLayout.InputElementDescs[3].InputSlot = 0;
    standardInputLayout.InputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    standardInputLayout.InputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    standardInputLayout.InputElementDescs[3].InstanceDataStepRate = 0;

    standardInputLayout.InputElementDescs[4].SemanticName = "BINORMAL";
    standardInputLayout.InputElementDescs[4].SemanticIndex = 0;
    standardInputLayout.InputElementDescs[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    standardInputLayout.InputElementDescs[4].InputSlot = 0;
    standardInputLayout.InputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    standardInputLayout.InputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    standardInputLayout.InputElementDescs[4].InstanceDataStepRate = 0;

    standardInputLayout.InputElementDescs[5].SemanticName = "COLOR";
    standardInputLayout.InputElementDescs[5].SemanticIndex = 0;
    standardInputLayout.InputElementDescs[5].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    standardInputLayout.InputElementDescs[5].InputSlot = 0;
    standardInputLayout.InputElementDescs[5].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    standardInputLayout.InputElementDescs[5].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    standardInputLayout.InputElementDescs[5].InstanceDataStepRate = 0;

    standardInputLayout.InputLayoutDesc.pInputElementDescs = standardInputLayout.InputElementDescs;
    standardInputLayout.InputLayoutDesc.NumElements = 6;
    
    mPipelineInputLayoutMap.insert(std::pair<ShaderInputLayoutType, ShaderPipelineInputLayout>(InputLayout_Standard, standardInputLayout));
}