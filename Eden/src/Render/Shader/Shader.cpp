#include "Render/Shader/Specialized/DynamicShader.h"
#include "Util/General/FileUtil.h"
#include "Render/Shader/ConstantBuffers/GlobalConstantBufferManager.h"

bool DynamicShader::mRebuildAllShaders = true;

DynamicShader::DynamicShader()
{
	mHasPixelShader = false;
	mUsesTessellation = false;
	mVertexShader = NULL;
	mPixelShader = NULL;
	mHullShader = NULL;
	mDomainShader = NULL;
	mLayout = NULL;
}

DynamicShader::~DynamicShader()
{
	if (mLayout)
	{
		mLayout->Release();
		mLayout = NULL;
	}
	if (mPixelShader)
	{
		mPixelShader->Release();
		mPixelShader = NULL;
	}
	if (mVertexShader)
	{
		mVertexShader->Release();
		mVertexShader = NULL;
	}
	if (mHullShader)
	{
		mHullShader->Release();
		mHullShader = NULL;
	}
	if (mDomainShader)
	{
		mDomainShader->Release();
		mDomainShader = NULL;
	}
}

void DynamicShader::Release()
{	
	for(uint32 i = 0; i < mVSConstantBuffers.CurrentSize(); i++)
	{
		mVSConstantBuffers[i]->Release();
		mVSConstantBuffers[i] = NULL;
	}
	mVSConstantBuffers.Clear();

	for(uint32 i = 0; i < mPSConstantBuffers.CurrentSize(); i++)
	{
		mPSConstantBuffers[i]->Release();
		mPSConstantBuffers[i] = NULL;
	}
	mPSConstantBuffers.Clear();

	for(uint32 i = 0; i < mHSConstantBuffers.CurrentSize(); i++)
	{
		mHSConstantBuffers[i]->Release();
		mHSConstantBuffers[i] = NULL;
	}
	mHSConstantBuffers.Clear();

	for(uint32 i = 0; i < mDSConstantBuffers.CurrentSize(); i++)
	{
		mDSConstantBuffers[i]->Release();
		mDSConstantBuffers[i] = NULL;
	}
	mDSConstantBuffers.Clear();

	mSamplerStates.Clear();
}


void DynamicShader::OutputShaderByteCodeToFile(std::string &shaderOutputName, void* data, unsigned int dataLength)
{
	std::ofstream outputFile(shaderOutputName, std::ios::binary);

	outputFile.write((char*)&dataLength, sizeof(dataLength));
	outputFile.write((char*)data, dataLength);

	outputFile.close();
}


void DynamicShader::ReadShaderByteCodeFromFile(std::string &shaderOutputName, void*& data, unsigned int &dataLength)
{
	std::ifstream inputFile(shaderOutputName, std::ios::binary);

	inputFile.read((char *)&dataLength, sizeof(dataLength));

	char *dataArray = new char[dataLength];

	inputFile.read((char *)dataArray, sizeof(char) * dataLength);

	data = dataArray;
}


bool DynamicShader::Initialize(ID3D11Device* device, HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename, WCHAR* hsFilename, WCHAR* dsFilename, LPCSTR vsEntry, LPCSTR psEntry, LPCSTR hsEntry, LPCSTR dsEntry,
	DynamicArray<D3D11_BUFFER_DESC> &vsConstBufferDescs, DynamicArray<D3D11_BUFFER_DESC> &psConstBufferDescs, 
	DynamicArray<D3D11_BUFFER_DESC> &hsConstBufferDescs, DynamicArray<D3D11_BUFFER_DESC> &dsConstBufferDescs, 
	DynamicArray<ID3D11SamplerState *> &samplers, bool hasPixelShader, bool usesTessellation, std::string shaderOutputName)
{
	HRESULT result;
	ID3D10Blob* errorMessage = NULL;
	ID3D10Blob* vertexShaderBuffer = NULL;
	ID3D10Blob* pixelShaderBuffer = NULL;
	ID3D10Blob* hullShaderBuffer = NULL;
	ID3D10Blob* domainShaderBuffer = NULL;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[6];
	unsigned int numElements;

	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "TEXCOORD";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	polygonLayout[2].SemanticName = "NORMAL";
	polygonLayout[2].SemanticIndex = 0;
	polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[2].InputSlot = 0;
	polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[2].InstanceDataStepRate = 0;

	polygonLayout[3].SemanticName = "TANGENT";
	polygonLayout[3].SemanticIndex = 0;
	polygonLayout[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[3].InputSlot = 0;
	polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[3].InstanceDataStepRate = 0;

	polygonLayout[4].SemanticName = "BINORMAL";
	polygonLayout[4].SemanticIndex = 0;
	polygonLayout[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[4].InputSlot = 0;
	polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[4].InstanceDataStepRate = 0;

	polygonLayout[5].SemanticName = "COLOR";
	polygonLayout[5].SemanticIndex = 0;
	polygonLayout[5].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[5].InputSlot = 0;
	polygonLayout[5].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[5].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[5].InstanceDataStepRate = 0;

	std::string outputLocation = "../EdensEngine/data/HLSL/Precompiled/";

	std::string vertexShaderName = outputLocation + shaderOutputName + vsEntry + ".ecs";

	if(!FileUtil::DoesFileExist(vertexShaderName) || mRebuildAllShaders)
	{
		result = D3DX11CompileFromFile(vsFilename, NULL, NULL, vsEntry, "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, 
			&vertexShaderBuffer, &errorMessage, NULL);

		if(FAILED(result))
		{
			if(errorMessage)
			{
				OutputShaderErrorMessage(errorMessage, hwnd, vsFilename);
			}
			else
			{
				MessageBox(hwnd, vsFilename, L"Missing Shader File", MB_OK);
			}
			return false;
		}
		else
		{
			OutputShaderByteCodeToFile(vertexShaderName, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize());
		}

		result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &mVertexShader);
		if(FAILED(result))
		{
			return false;
		}

		numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
		result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &mLayout);
		if(FAILED(result))
		{
			return false;
		}

		vertexShaderBuffer->Release();
		vertexShaderBuffer = 0;
	}
	else
	{
		void *data = NULL;
		unsigned int dataLength = 0;

		ReadShaderByteCodeFromFile(vertexShaderName, data, dataLength);

		result = device->CreateVertexShader(data, dataLength, NULL, &mVertexShader);
		if(FAILED(result))
		{
			return false;
		}

		numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
		result = device->CreateInputLayout(polygonLayout, numElements, data, dataLength, &mLayout);
		if(FAILED(result))
		{
			return false;
		}

		delete [] data;
	}
	


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
	}


	for(uint32 i = 0; i < vsConstBufferDescs.CurrentSize(); i++)
	{
		ID3D11Buffer* vertexShaderConstBuffer;

		result = device->CreateBuffer(&vsConstBufferDescs[i], NULL, &vertexShaderConstBuffer);
		if(FAILED(result))
		{
			return false;
		}

		mVSConstantBuffers.Add(vertexShaderConstBuffer);
	}

	if(hasPixelShader)
	{
		for(uint32 i = 0; i < psConstBufferDescs.CurrentSize(); i++)
		{
			ID3D11Buffer* pixelShaderConstBuffer;

			result = device->CreateBuffer(&psConstBufferDescs[i], NULL, &pixelShaderConstBuffer);
			if(FAILED(result))
			{
				return false;
			}

			mPSConstantBuffers.Add(pixelShaderConstBuffer);
		}
	}

	if(usesTessellation)
	{
		for(uint32 i = 0; i < hsConstBufferDescs.CurrentSize(); i++)
		{
			ID3D11Buffer* hullShaderConstBuffer;

			result = device->CreateBuffer(&hsConstBufferDescs[i], NULL, &hullShaderConstBuffer);
			if(FAILED(result))
			{
				return false;
			}

			mHSConstantBuffers.Add(hullShaderConstBuffer);
		}

		for(uint32 i = 0; i < dsConstBufferDescs.CurrentSize(); i++)
		{
			ID3D11Buffer* domainShaderConstBuffer;

			result = device->CreateBuffer(&dsConstBufferDescs[i], NULL, &domainShaderConstBuffer);
			if(FAILED(result))
			{
				return false;
			}

			mDSConstantBuffers.Add(domainShaderConstBuffer);
		}
	}

	for(uint32 i = 0; i < samplers.CurrentSize(); i++)
	{
		mSamplerStates.Add(samplers[i]);
	}

	mUsesTessellation = usesTessellation;
	mHasPixelShader = hasPixelShader;

	return true;
}

bool DynamicShader::SetShaderParameters(ID3D11DeviceContext* deviceContext, ShaderParams &shaderParams, ShaderResources &shaderResources)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	int floatCountIndex = 0;

	for(uint32 i = 0; i < shaderResources.vsResources.CurrentSize(); i++)
	{
		deviceContext->VSSetShaderResources(i, 1, &shaderResources.vsResources[i]);
	}

	if(mHasPixelShader)
	{
		for(uint32 i = 0; i < shaderResources.psResources.CurrentSize(); i++)
		{
			deviceContext->PSSetShaderResources(i, 1, &shaderResources.psResources[i]);
		}
	}
	
	uint32 vsBufferOffset = 0;

	for (uint32 i = 0; i < shaderResources.vsGlobalBuffers.CurrentSize(); i++)
	{
		deviceContext->VSSetConstantBuffers(vsBufferOffset, 1, &shaderResources.vsGlobalBuffers[i]);
		vsBufferOffset++;
	}

	for (uint32 i = 0; i < shaderResources.vsMaterialBuffers.CurrentSize(); i++)
	{
		deviceContext->VSSetConstantBuffers(vsBufferOffset, 1, &shaderResources.vsMaterialBuffers[i]);
		vsBufferOffset++;
	}

	for (uint32 i = 0; i < shaderResources.vsObjectBuffers.CurrentSize(); i++)
	{
		deviceContext->VSSetConstantBuffers(vsBufferOffset, 1, &shaderResources.vsObjectBuffers[i]);
		vsBufferOffset++;
	}

	for(uint32 i = 0; i < mVSConstantBuffers.CurrentSize(); i++)
	{
		result = deviceContext->Map(mVSConstantBuffers[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if(FAILED(result))
		{
			return false;
		}

		memcpy(mappedResource.pData, shaderResources.vsData[i], sizeof(float) * shaderResources.floatCounts[floatCountIndex]);
		floatCountIndex++;

		deviceContext->Unmap(mVSConstantBuffers[i], 0);
		deviceContext->VSSetConstantBuffers(i + vsBufferOffset, 1, &mVSConstantBuffers[i]);
	}

	if(mUsesTessellation)
	{
		for(uint32 i = 0; i < mHSConstantBuffers.CurrentSize(); i++)
		{
			result = deviceContext->Map(mHSConstantBuffers[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			if(FAILED(result))
			{
				return false;
			}

			memcpy(mappedResource.pData, shaderResources.hsData[i], sizeof(float) * shaderResources.floatCounts[floatCountIndex]);
			floatCountIndex++;

			deviceContext->Unmap(mHSConstantBuffers[i], 0);
			deviceContext->HSSetConstantBuffers(i, 1, &mHSConstantBuffers[i]);
		}

		for(uint32 i = 0; i < shaderResources.dsResources.CurrentSize(); i++)
		{
			deviceContext->DSSetShaderResources(i, 1, &shaderResources.dsResources[i]);
		}

		for(uint32 i = 0; i < mDSConstantBuffers.CurrentSize(); i++)
		{
			result = deviceContext->Map(mDSConstantBuffers[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			if(FAILED(result))
			{
				return false;
			}

			memcpy(mappedResource.pData, shaderResources.dsData[i], sizeof(float) * shaderResources.floatCounts[floatCountIndex]);
			floatCountIndex++;

			deviceContext->Unmap(mDSConstantBuffers[i], 0);
			deviceContext->DSSetConstantBuffers(i, 1, &mDSConstantBuffers[i]);
		}
	}

	if(mHasPixelShader)
	{
		uint32 psBufferOffset = 0;

		for (uint32 i = 0; i < shaderResources.psGlobalBuffers.CurrentSize(); i++)
		{
			deviceContext->PSSetConstantBuffers(psBufferOffset, 1, &shaderResources.psGlobalBuffers[i]);
			psBufferOffset++;
		}

		for (uint32 i = 0; i < shaderResources.psMaterialBuffers.CurrentSize(); i++)
		{
			deviceContext->PSSetConstantBuffers(psBufferOffset, 1, &shaderResources.psMaterialBuffers[i]);
			psBufferOffset++;
		}

		for (uint32 i = 0; i < shaderResources.psObjectBuffers.CurrentSize(); i++)
		{
			deviceContext->PSSetConstantBuffers(psBufferOffset, 1, &shaderResources.psObjectBuffers[i]);
			psBufferOffset++;
		}

		for(uint32 i = 0; i < mPSConstantBuffers.CurrentSize(); i++)
		{
			result = deviceContext->Map(mPSConstantBuffers[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			if(FAILED(result))
			{
				return false;
			}

			memcpy(mappedResource.pData, shaderResources.psData[i], sizeof(float) * shaderResources.floatCounts[floatCountIndex]);
			floatCountIndex++;

			deviceContext->Unmap(mPSConstantBuffers[i], 0);
			deviceContext->PSSetConstantBuffers(i + psBufferOffset, 1, &mPSConstantBuffers[i]);
		}
	}

	return true;
}


bool DynamicShader::Render(ID3D11DeviceContext* deviceContext, ID3D11SamplerState *overrideSampler /* = NULL */)
{
	deviceContext->IASetInputLayout(mLayout);
	deviceContext->VSSetShader(mVertexShader, NULL, 0);
	
	if(mUsesTessellation)
	{
		deviceContext->HSSetShader(mHullShader, NULL, 0);
		deviceContext->DSSetShader(mDomainShader, NULL, 0);
	}
	else
	{
		deviceContext->HSSetShader(NULL, NULL, 0);
		deviceContext->DSSetShader(NULL, NULL, 0);
	}

	if(mHasPixelShader)
	{
		deviceContext->PSSetShader(mPixelShader, NULL, 0);

		for(uint32 i = 0; i < mSamplerStates.CurrentSize(); i++)
		{
			if (overrideSampler)
			{
				deviceContext->PSSetSamplers(i, 1, &overrideSampler);
			}
			else
			{
				deviceContext->PSSetSamplers(i, 1, &mSamplerStates[i]);
			}
		}
	}
	else
	{
		deviceContext->PSSetShader(NULL, NULL, 0);
	}
	
	return true;
}

void DynamicShader::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename)
{
	char* compileErrors;
	unsigned long bufferSize, i;
	std::ofstream fout;

	compileErrors = (char*)(errorMessage->GetBufferPointer());
	bufferSize = errorMessage->GetBufferSize();
	fout.open("shader-error.txt");

	for (i = 0; i < bufferSize; i++)
	{
		fout << compileErrors[i];
	}
	fout.close();
	errorMessage->Release();
	errorMessage = NULL;
	MessageBox(hwnd, L"Error compiling shader.  Check shader-error.txt for message.", shaderFilename, MB_OK);
}