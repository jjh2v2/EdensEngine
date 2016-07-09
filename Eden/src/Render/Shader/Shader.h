#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"
#include "Render/DirectX/D3D12Helper.h"

struct ShaderInitializationData
{
	ShaderInitializationData()
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
};

class Shader
{
public:
	Shader();
	~Shader();

	bool Initialize(ID3D12Device* device, ShaderInitializationData &initData);

	void ReadShaderByteCodeFromFile(const std::string &shaderOutputName, void *&data, unsigned int &dataLength);
	void OutputShaderByteCodeToFile(const std::string &shaderOutputName, void* data, unsigned int dataLength);
	ID3DBlob *GetShaderCode(const std::string &compiledShaderFileLocation, WCHAR* shaderFilePath, LPCSTR entryPoint, LPCSTR target, UINT flags, const D3D_SHADER_MACRO *defines);

private:
	bool mUsesTessellation;
	bool mHasPixelShader;

	ID3D12RootSignature *mRootSignature;
	ID3D12PipelineState *mPipelineState;
	//ID3D11InputLayout* mLayout;
};