#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"

struct ShaderResources
{
	DynamicArray<float*> vsData;
	DynamicArray<float*> psData;
	DynamicArray<float*> hsData;
	DynamicArray<float*> dsData;
	DynamicArray<int> floatCounts;
	DynamicArray<ID3D11ShaderResourceView*> vsResources;
	DynamicArray<ID3D11ShaderResourceView*> psResources;
	DynamicArray<ID3D11ShaderResourceView*> dsResources;

	DynamicArray<ID3D11Buffer*> vsGlobalBuffers;
	DynamicArray<ID3D11Buffer*> vsMaterialBuffers;
	DynamicArray<ID3D11Buffer*> vsObjectBuffers;

	DynamicArray<ID3D11Buffer*> psGlobalBuffers;
	DynamicArray<ID3D11Buffer*> psMaterialBuffers;
	DynamicArray<ID3D11Buffer*> psObjectBuffers;
};

class DynamicShader
{
public:
	DynamicShader();
	~DynamicShader();

	bool Initialize(ID3D11Device* device, HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename, WCHAR* hsFilename, WCHAR* dsFilename, LPCSTR vsEntry, LPCSTR psEntry, LPCSTR hsEntry, LPCSTR dsEntry,
		DynamicArray<D3D11_BUFFER_DESC> &vsConstBufferDescs, DynamicArray<D3D11_BUFFER_DESC> &psConstBufferDescs, 
		DynamicArray<D3D11_BUFFER_DESC> &hsConstBufferDescs, DynamicArray<D3D11_BUFFER_DESC> &dsConstBufferDescs, 
		DynamicArray<ID3D11SamplerState *> &samplers, bool hasPixelShader, bool usesTessellation, std::string shaderOutputName);

	void ReadShaderByteCodeFromFile(std::string &shaderOutputName, void *&data, unsigned int &dataLength);
	void OutputShaderByteCodeToFile(std::string &shaderOutputName, void* data, unsigned int dataLength);

	void Release();

	bool SetShaderParameters(ID3D11DeviceContext* deviceContext, ShaderParams &shaderParams, ShaderResources &shaderResources);

	bool Render(ID3D11DeviceContext* deviceContext, ID3D11SamplerState *overrideSampler = NULL);

	void OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename);

private:
	static bool mRebuildAllShaders;

	DynamicArray<ID3D11Buffer*> mVSConstantBuffers;
	DynamicArray<ID3D11Buffer*> mHSConstantBuffers;
	DynamicArray<ID3D11Buffer*> mDSConstantBuffers;
	DynamicArray<ID3D11Buffer*> mPSConstantBuffers;
	DynamicArray<ID3D11SamplerState*> mSamplerStates;
	bool mUsesTessellation;
	bool mHasPixelShader;

	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;
	ID3D11HullShader* mHullShader;
	ID3D11DomainShader* mDomainShader;
	ID3D11InputLayout* mLayout;
};