#include "Render/Shader/Shader.h"
#include "Util/File/FileUtil.h"
#include <fstream>

Shader::Shader()
{
	mHasPixelShader = false;
	mUsesTessellation = false;
}

Shader::~Shader()
{
	
}


void Shader::OutputShaderByteCodeToFile(const std::string &shaderOutputName, void* data, unsigned int dataLength)
{
	std::ofstream outputFile(shaderOutputName, std::ios::binary);

	outputFile.write((char*)&dataLength, sizeof(dataLength));
	outputFile.write((char*)data, dataLength);

	outputFile.close();
}


void Shader::ReadShaderByteCodeFromFile(const std::string &shaderOutputName, void*& data, unsigned int &dataLength)
{
	std::ifstream inputFile(shaderOutputName, std::ios::binary);

	inputFile.read((char *)&dataLength, sizeof(dataLength));

	char *dataArray = new char[dataLength];

	inputFile.read((char *)dataArray, sizeof(char) * dataLength);

	data = dataArray;
}

bool Shader::GetShaderCode(const std::string &shaderFileLocation, WCHAR* shaderFilename)
{
	if (!FileUtil::DoesFileExist(shaderFileLocation) || ApplicationSpecification::RebuildAllShaders)
	{
		result = D3DCompileFromFile(shaderFilename, defines, NULL, vsEntry, "vs_5_0", shaderFlags, 0, &vertexShaderBuffer, &errorMessage);

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
			return false;
		}
		else
		{
			D3D_SHADER_DATA shaderData;
			shaderData.pBytecode = vertexShaderBuffer->GetBufferPointer();
			shaderData.BytecodeLength = vertexShaderBuffer->GetBufferSize();
			ID3DBlob* compressedShader;
			Direct3DUtils::ThrowIfHRESULTFailed(D3DCompressShaders(1, &shaderData, D3D_COMPRESS_SHADER_KEEP_ALL_PARTS, &compressedShader));
			OutputShaderByteCodeToFile(shaderFileLocation, compressedShader->GetBufferPointer(), compressedShader->GetBufferSize());
		}
	}
	else
	{
		void *data = NULL;
		unsigned int dataLength = 0;

		ReadShaderByteCodeFromFile(shaderFileLocation, data, dataLength);

		ID3DBlob* decompressedShader[1] = { NULL };
		uint32 indices[1] = { 0 };
		Direct3DUtils::ThrowIfHRESULTFailed(D3DDecompressShaders(data, dataLength, 1, 0,
			indices, 0, decompressedShader, NULL));

		delete[] data;
	}
}

bool Shader::Initialize(ID3D12Device* device, HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename, WCHAR* hsFilename, WCHAR* dsFilename, LPCSTR vsEntry, LPCSTR psEntry, LPCSTR hsEntry, LPCSTR dsEntry,
	bool hasPixelShader, bool usesTessellation, std::string shaderOutputName, const D3D_SHADER_MACRO *defines)
{

	HRESULT result;
	ID3DBlob* errorMessage = NULL;
	ID3DBlob* vertexShaderBuffer = NULL;
	ID3DBlob* pixelShaderBuffer = NULL;
	ID3DBlob* hullShaderBuffer = NULL;
	ID3DBlob* domainShaderBuffer = NULL;
	D3D12_INPUT_ELEMENT_DESC polygonLayout[6];
	//unsigned int numElements;

	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "TEXCOORD";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	polygonLayout[2].SemanticName = "NORMAL";
	polygonLayout[2].SemanticIndex = 0;
	polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[2].InputSlot = 0;
	polygonLayout[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	polygonLayout[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	polygonLayout[2].InstanceDataStepRate = 0;

	polygonLayout[3].SemanticName = "TANGENT";
	polygonLayout[3].SemanticIndex = 0;
	polygonLayout[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[3].InputSlot = 0;
	polygonLayout[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	polygonLayout[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	polygonLayout[3].InstanceDataStepRate = 0;

	polygonLayout[4].SemanticName = "BINORMAL";
	polygonLayout[4].SemanticIndex = 0;
	polygonLayout[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[4].InputSlot = 0;
	polygonLayout[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	polygonLayout[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	polygonLayout[4].InstanceDataStepRate = 0;

	polygonLayout[5].SemanticName = "COLOR";
	polygonLayout[5].SemanticIndex = 0;
	polygonLayout[5].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[5].InputSlot = 0;
	polygonLayout[5].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	polygonLayout[5].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	polygonLayout[5].InstanceDataStepRate = 0;

	std::string outputLocation = "../EdensEngine/data/HLSL/Precompiled/";

	std::string vertexShaderName = outputLocation + shaderOutputName + vsEntry + ".ecs";

	UINT shaderFlags = D3DCOMPILE_WARNINGS_ARE_ERRORS;

	
	

	/*
	if(hasPixelShader)
	{
		std::string pixelShaderName = outputLocation + shaderOutputName + psEntry + ".ecs";

		if(!FileUtil::DoesFileExist(pixelShaderName) || mRebuildAllShaders)
		{
			result = D3DX11CompileFromFile(psFilename, NULL, NULL, psEntry, "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, 
				&pixelShaderBuffer, &errorMessage, NULL);

			if(FAILED(result))
			{
				if(errorMessage)
				{
					OutputShaderErrorMessage(errorMessage, hwnd, psFilename);
				}
				else
				{
					MessageBox(hwnd, psFilename, L"Missing Shader File", MB_OK);
				}

				return false;
			}
			else
			{
				OutputShaderByteCodeToFile(pixelShaderName, pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize());
			}

			result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &mPixelShader);
			if(FAILED(result))
			{
				return false;
			}

			pixelShaderBuffer->Release();
			pixelShaderBuffer = 0;
		}
		else
		{
			void *data = NULL;
			unsigned int dataLength = 0;

			ReadShaderByteCodeFromFile(pixelShaderName, data, dataLength);

			result = device->CreatePixelShader(data, dataLength, NULL, &mPixelShader);
			if(FAILED(result))
			{
				return false;
			}

			delete [] data;
		}
	}
	

	if(usesTessellation)
	{
		std::string hullShaderName = outputLocation + shaderOutputName + hsEntry + ".ecs";
		std::string domainShaderName = outputLocation + shaderOutputName + dsEntry + ".ecs";

		if(!FileUtil::DoesFileExist(hullShaderName) || mRebuildAllShaders)
		{
			result = D3DX11CompileFromFile(hsFilename, NULL, NULL, hsEntry, "hs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, 
				&hullShaderBuffer, &errorMessage, NULL);

			if(FAILED(result))
			{
				if(errorMessage)
				{
					OutputShaderErrorMessage(errorMessage, hwnd, hsFilename);
				}
				else
				{
					MessageBox(hwnd, hsFilename, L"Missing Shader File", MB_OK);
				}

				return false;
			}
			else
			{
				OutputShaderByteCodeToFile(hullShaderName, hullShaderBuffer->GetBufferPointer(), hullShaderBuffer->GetBufferSize());
			}

			result = device->CreateHullShader(hullShaderBuffer->GetBufferPointer(), hullShaderBuffer->GetBufferSize(), NULL, &mHullShader);
			if(FAILED(result))
			{
				return false;
			}

			hullShaderBuffer->Release();
			hullShaderBuffer = 0;
		}
		else
		{
			void *data = NULL;
			unsigned int dataLength = 0;

			ReadShaderByteCodeFromFile(hullShaderName, data, dataLength);

			result = device->CreateHullShader(data, dataLength, NULL, &mHullShader);
			if(FAILED(result))
			{
				return false;
			}

			delete [] data;
		}

		if(!FileUtil::DoesFileExist(domainShaderName) || mRebuildAllShaders)
		{
			result = D3DX11CompileFromFile(dsFilename, NULL, NULL, dsEntry, "ds_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, 
				&domainShaderBuffer, &errorMessage, NULL);

			if(FAILED(result))
			{
				if(errorMessage)
				{
					OutputShaderErrorMessage(errorMessage, hwnd, dsFilename);
				}
				else
				{
					MessageBox(hwnd, dsFilename, L"Missing Shader File", MB_OK);
				}

				return false;
			}
			else
			{
				OutputShaderByteCodeToFile(domainShaderName, domainShaderBuffer->GetBufferPointer(), domainShaderBuffer->GetBufferSize());
			}

			result = device->CreateDomainShader(domainShaderBuffer->GetBufferPointer(), domainShaderBuffer->GetBufferSize(), NULL, &mDomainShader);
			if(FAILED(result))
			{
				return false;
			}

			domainShaderBuffer->Release();
			domainShaderBuffer = 0;
		}
		else
		{
			void *data = NULL;
			unsigned int dataLength = 0;

			ReadShaderByteCodeFromFile(domainShaderName, data, dataLength);

			result = device->CreateDomainShader(data, dataLength, NULL, &mDomainShader);
			if(FAILED(result))
			{
				return false;
			}

			delete [] data;
		}
	}*/

	mUsesTessellation = usesTessellation;
	mHasPixelShader = hasPixelShader;

	return true;
}