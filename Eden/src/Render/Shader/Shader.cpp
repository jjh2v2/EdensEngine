#include "Render/Shader/Shader.h"
#include "Util/File/FileUtil.h"
#include <fstream>

Shader::Shader(ID3D12Device* device, ShaderPipelineDefinition &initData)
{
	mHasPixelShader = false;
	mUsesTessellation = false;
	mVertexShader = NULL;
	mPixelShader = NULL;
	mHullShader = NULL;
	mDomainShader = NULL;

	Initialize(device, initData);
}

Shader::~Shader()
{
	if (mVertexShader)
	{
		mVertexShader->Release();
		mVertexShader = NULL;
	}
	
	if (mPixelShader)
	{
		mPixelShader->Release();
		mPixelShader = NULL;
	}

	if (mDomainShader)
	{
		mDomainShader->Release();
		mDomainShader = NULL;
	}

	if (mHullShader)
	{
		mHullShader->Release();
		mHullShader = NULL;
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
		HRESULT result = D3DCompileFromFile(shaderFilePath, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target, flags, 0, &compiledShader, &errorMessage);

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
	mInputElementDescs[0].SemanticName = "POSITION";
	mInputElementDescs[0].SemanticIndex = 0;
	mInputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	mInputElementDescs[0].InputSlot = 0;
	mInputElementDescs[0].AlignedByteOffset = 0;
	mInputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	mInputElementDescs[0].InstanceDataStepRate = 0;

	mInputElementDescs[1].SemanticName = "TEXCOORD";
	mInputElementDescs[1].SemanticIndex = 0;
	mInputElementDescs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	mInputElementDescs[1].InputSlot = 0;
	mInputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	mInputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	mInputElementDescs[1].InstanceDataStepRate = 0;

	mInputElementDescs[2].SemanticName = "NORMAL";
	mInputElementDescs[2].SemanticIndex = 0;
	mInputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	mInputElementDescs[2].InputSlot = 0;
	mInputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	mInputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	mInputElementDescs[2].InstanceDataStepRate = 0;

	mInputElementDescs[3].SemanticName = "TANGENT";
	mInputElementDescs[3].SemanticIndex = 0;
	mInputElementDescs[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	mInputElementDescs[3].InputSlot = 0;
	mInputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	mInputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	mInputElementDescs[3].InstanceDataStepRate = 0;

	mInputElementDescs[4].SemanticName = "BINORMAL";
	mInputElementDescs[4].SemanticIndex = 0;
	mInputElementDescs[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	mInputElementDescs[4].InputSlot = 0;
	mInputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	mInputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	mInputElementDescs[4].InstanceDataStepRate = 0;

	mInputElementDescs[5].SemanticName = "COLOR";
	mInputElementDescs[5].SemanticIndex = 0;
	mInputElementDescs[5].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	mInputElementDescs[5].InputSlot = 0;
	mInputElementDescs[5].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	mInputElementDescs[5].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	mInputElementDescs[5].InstanceDataStepRate = 0;

	mInputLayoutDesc.pInputElementDescs = mInputElementDescs;
	mInputLayoutDesc.NumElements = 6;

	std::string outputLocation = "../Eden/data/HLSL/Precompiled/";
	UINT shaderFlags = D3DCOMPILE_WARNINGS_ARE_ERRORS;

	std::string vertexShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.VSEntry + ".ecs";
	mVertexShader = GetShaderCode(vertexShaderCompiledLocation, initData.VSFilePath, initData.VSEntry.c_str(), "vs_5_0", shaderFlags, initData.Defines);
	
	if (initData.HasPixelShader)
	{
		std::string pixelShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.PSEntry + ".ecs";
		mPixelShader = GetShaderCode(pixelShaderCompiledLocation, initData.PSFilePath, initData.PSEntry.c_str(), "ps_5_0", shaderFlags, initData.Defines);
	}

	if (initData.UsesTessellation)
	{
		std::string hullShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.HSEntry + ".ecs";
		std::string domainShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.DSEntry + ".ecs";
		mHullShader = GetShaderCode(hullShaderCompiledLocation, initData.HSFilePath, initData.HSEntry.c_str(), "hs_5_0", shaderFlags, initData.Defines);
		mDomainShader = GetShaderCode(domainShaderCompiledLocation, initData.DSFilePath, initData.DSEntry.c_str(), "ds_5_0", shaderFlags, initData.Defines);
	}

	mUsesTessellation = initData.UsesTessellation;
	mHasPixelShader = initData.HasPixelShader;

	return true;
}