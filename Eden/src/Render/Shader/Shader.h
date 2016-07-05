#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"

class Shader
{
public:
	Shader();
	~Shader();

	bool Initialize(ID3D12Device* device, HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename, WCHAR* hsFilename, WCHAR* dsFilename, LPCSTR vsEntry, LPCSTR psEntry, LPCSTR hsEntry, LPCSTR dsEntry,
		bool hasPixelShader, bool usesTessellation, std::string shaderOutputName, const D3D_SHADER_MACRO *defines);

	void ReadShaderByteCodeFromFile(const std::string &shaderOutputName, void *&data, unsigned int &dataLength);
	void OutputShaderByteCodeToFile(const std::string &shaderOutputName, void* data, unsigned int dataLength);
	bool GetShaderCode();

private:
	bool mUsesTessellation;
	bool mHasPixelShader;

	//ID3D11InputLayout* mLayout;
};