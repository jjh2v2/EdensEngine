#include "Render/Shader/Shader.h"
#include "Util/File/FileUtil.h"
#include <fstream>

Shader::Shader(ID3D12Device* device, ShaderPipelineDefinition &initData)
{
	mVertexShader = NULL;
	mPixelShader = NULL;
	mHullShader = NULL;
	mDomainShader = NULL;
    mComputeShader = NULL;

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

    if (mComputeShader)
    {
        mComputeShader->Release();
        mComputeShader = NULL;
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
	std::string outputLocation = "../Eden/data/HLSL/Precompiled/";
	UINT shaderFlags = D3DCOMPILE_WARNINGS_ARE_ERRORS;

    if (initData.IsRenderShader)
    {
        std::string vertexShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.VSEntry + ".ecs";
        mVertexShader = GetShaderCode(vertexShaderCompiledLocation, initData.VSFilePath, initData.VSEntry.c_str(), "vs_5_0", shaderFlags, initData.Defines);

        if (initData.HasPixelShader)
        {
            std::string pixelShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.PSEntry + ".ecs";
            mPixelShader = GetShaderCode(pixelShaderCompiledLocation, initData.PSFilePath, initData.PSEntry.c_str(), "ps_5_0", shaderFlags, initData.Defines);
        }

        if (initData.HasTessellation)
        {
            std::string hullShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.HSEntry + ".ecs";
            std::string domainShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.DSEntry + ".ecs";
            mHullShader = GetShaderCode(hullShaderCompiledLocation, initData.HSFilePath, initData.HSEntry.c_str(), "hs_5_0", shaderFlags, initData.Defines);
            mDomainShader = GetShaderCode(domainShaderCompiledLocation, initData.DSFilePath, initData.DSEntry.c_str(), "ds_5_0", shaderFlags, initData.Defines);
        }
    }
    else
    {
        std::string computeShaderCompiledLocation = outputLocation + initData.ShaderOutputName + initData.CSEntry + ".ecs";
        mComputeShader = GetShaderCode(computeShaderCompiledLocation, initData.CSFilePath, initData.CSEntry.c_str(), "cs_5_0", shaderFlags, initData.Defines);
    }

	return true;
}