#include "Render/Shader/Shader.h"
#include "Util/File/FileUtil.h"
#include <fstream>

Shader::Shader(ID3D12Device* device, ShaderPipelineDefinition &initData)
{
	mHasPixelShader = false;
	mUsesTessellation = false;
	mRootSignature = NULL;
	mPipelineState = NULL;

	Initialize(device, initData);
}

Shader::~Shader()
{
	if (mRootSignature)
	{
		mRootSignature->Release();
		mRootSignature = NULL;
	}
}


void Shader::OutputShaderByteCodeToFile(const std::string &shaderOutputName, void* data, uint32 dataLength)
{
	std::ofstream outputFile(shaderOutputName, std::ios::binary);

	outputFile.write((char*)&dataLength, sizeof(dataLength));
	outputFile.write((char*)data, dataLength);

	outputFile.close();
}


void Shader::ReadShaderByteCodeFromFile(const std::string &shaderOutputName, void*& data, uint32 &dataLength)
{
	std::ifstream inputFile(shaderOutputName, std::ios::binary);

	inputFile.read((char *)&dataLength, sizeof(dataLength));

	char *dataArray = new char[dataLength];

	inputFile.read((char *)dataArray, sizeof(char) * dataLength);

	data = dataArray;
}

ID3DBlob *Shader::GetShaderCode(const std::string &compiledShaderFileLocation, WCHAR* shaderFilePath, LPCSTR entryPoint, LPCSTR target, UINT flags, const D3D_SHADER_MACRO *defines)
{
	ID3DBlob* errorMessage = NULL;
	ID3DBlob* compiledShader = NULL;

	if (!FileUtil::DoesFileExist(compiledShaderFileLocation) || ApplicationSpecification::RebuildAllShaders)
	{
		HRESULT result = D3DCompileFromFile(shaderFilePath, defines, NULL, entryPoint, target, flags, 0, &compiledShader, &errorMessage);

		if (FAILED(result))
		{
			if (errorMessage)
			{
				Direct3DUtils::OutputShaderCompileError(errorMessage);
				Direct3DUtils::ThrowRuntimeError("Error while compiling shader. Check shader-error.txt");
			}
			else
			{
				Direct3DUtils::ThrowRuntimeError("Missing shader file.");
			}
			return NULL;
		}
		else
		{
			D3D_SHADER_DATA shaderData;
			shaderData.pBytecode = compiledShader->GetBufferPointer();
			shaderData.BytecodeLength = compiledShader->GetBufferSize();
			ID3DBlob* compressedShader;
			Direct3DUtils::ThrowIfHRESULTFailed(D3DCompressShaders(1, &shaderData, D3D_COMPRESS_SHADER_KEEP_ALL_PARTS, &compressedShader));
			OutputShaderByteCodeToFile(compiledShaderFileLocation, compressedShader->GetBufferPointer(), (uint32)compressedShader->GetBufferSize());
			compressedShader->Release();
			compressedShader = NULL;
		}
	}
	else
	{
		void *data = NULL;
		unsigned int dataLength = 0;

		ReadShaderByteCodeFromFile(compiledShaderFileLocation, data, dataLength);

		ID3DBlob* decompressedShader[1] = { compiledShader };
		uint32 indices[1] = { 0 };
		Direct3DUtils::ThrowIfHRESULTFailed(D3DDecompressShaders(data, dataLength, 1, 0,
			indices, 0, decompressedShader, NULL));

		delete[] data;
	}

	return compiledShader;
}

bool Shader::Initialize(ID3D12Device* device, ShaderPipelineDefinition &initData)
{
	ID3DBlob* vertexShader = NULL;
	ID3DBlob* pixelShader = NULL;
	ID3DBlob* hullShader = NULL;
	ID3DBlob* domainShader = NULL;
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[6];

	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[0].InputSlot = 0;
	inputElementDescs[0].AlignedByteOffset = 0;
	inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDescs[0].InstanceDataStepRate = 0;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[1].InputSlot = 0;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDescs[1].InstanceDataStepRate = 0;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].InputSlot = 0;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDescs[2].InstanceDataStepRate = 0;

	inputElementDescs[3].SemanticName = "TANGENT";
	inputElementDescs[3].SemanticIndex = 0;
	inputElementDescs[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[3].InputSlot = 0;
	inputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDescs[3].InstanceDataStepRate = 0;

	inputElementDescs[4].SemanticName = "BINORMAL";
	inputElementDescs[4].SemanticIndex = 0;
	inputElementDescs[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[4].InputSlot = 0;
	inputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDescs[4].InstanceDataStepRate = 0;

	inputElementDescs[5].SemanticName = "COLOR";
	inputElementDescs[5].SemanticIndex = 0;
	inputElementDescs[5].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[5].InputSlot = 0;
	inputElementDescs[5].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[5].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDescs[5].InstanceDataStepRate = 0;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = 6;

	std::string outputLocation = "../EdensEngine/data/HLSL/Precompiled/";
	UINT shaderFlags = D3DCOMPILE_WARNINGS_ARE_ERRORS;

	std::string vertexShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.VSEntry + ".ecs";
	vertexShader = GetShaderCode(vertexShaderCompiledLocation, initData.VSFilePath, initData.VSEntry, "vs_5_0", shaderFlags, initData.Defines);
	
	if (initData.HasPixelShader)
	{
		std::string pixelShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.PSEntry + ".ecs";
		pixelShader = GetShaderCode(pixelShaderCompiledLocation, initData.PSFilePath, initData.PSEntry, "ps_5_0", shaderFlags, initData.Defines);
	}

	if (initData.UsesTessellation)
	{
		std::string hullShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.HSEntry + ".ecs";
		std::string domainShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.DSEntry + ".ecs";
		hullShader = GetShaderCode(hullShaderCompiledLocation, initData.HSFilePath, initData.HSEntry, "hs_5_0", shaderFlags, initData.Defines);
		domainShader = GetShaderCode(domainShaderCompiledLocation, initData.DSFilePath, initData.DSEntry, "ds_5_0", shaderFlags, initData.Defines);
	}

	mUsesTessellation = initData.UsesTessellation;
	mHasPixelShader = initData.HasPixelShader;
	
	ID3DBlob* rootSignature;
	ID3DBlob* rootSignatureError;
	Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&initData.RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignature, &rootSignatureError));
	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, rootSignature->GetBufferPointer(), rootSignature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.InputLayout = inputLayoutDesc;
	pipelineStateDesc.pRootSignature = mRootSignature;
	pipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	if (mHasPixelShader)
	{
		pipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
	}
	if (mUsesTessellation)
	{
		pipelineStateDesc.HS = CD3DX12_SHADER_BYTECODE(hullShader);
		pipelineStateDesc.DS = CD3DX12_SHADER_BYTECODE(domainShader);
	}
	pipelineStateDesc.RasterizerState = initData.RasterDesc;
	pipelineStateDesc.BlendState = initData.BlendDesc;
	pipelineStateDesc.DepthStencilState = initData.DepthStencilDesc;
	pipelineStateDesc.SampleMask = UINT_MAX;
	pipelineStateDesc.PrimitiveTopologyType = initData.Topology;
	
	pipelineStateDesc.NumRenderTargets = initData.NumRenderTargets;
	for (UINT i = 0; i < initData.NumRenderTargets; i++)
	{
		pipelineStateDesc.RTVFormats[i] = initData.RenderTargetFormats[i];
	}

	pipelineStateDesc.DSVFormat = initData.DepthStencilFormat;
	pipelineStateDesc.SampleDesc.Count = 1;

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&mPipelineState)));

	return true;
}