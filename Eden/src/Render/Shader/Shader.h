#pragma once
#include "Render/Shader/ShaderPipelineDefinition.h"

/*
Remove any permutation references to the defines/macros because we're using cbuffer values for that now (also any defines should be done in separate #define shader files, rather than try to handle them in code)
Also need to compile shaders separately from root signatures. We can reuse the same shader code and just pass it in to create a pipeline with 
render target and render state settings.
*/


class Shader
{
public:
	Shader(ID3D12Device* device, ShaderPipelineDefinition &initData);
	~Shader();

	ID3DBlob *GetVertexShader() { return mVertexShader; }
	ID3DBlob *GetPixelShader() { return mPixelShader; }
	ID3DBlob *GetHullShader() { return mHullShader; }
	ID3DBlob *GetDomainShader() { return mDomainShader; }
	const D3D12_INPUT_LAYOUT_DESC &GetInputLayoutDesc() { return mInputLayoutDesc; }

private:
	bool Initialize(ID3D12Device* device, ShaderPipelineDefinition &initData);
	void ReadShaderByteCodeFromFile(const std::string &shaderOutputName, void *&data, uint32 &dataLength);
	void OutputShaderByteCodeToFile(const std::string &shaderOutputName, void* data, uint32 dataLength);
	ID3DBlob *GetShaderCode(const std::string &compiledShaderFileLocation, WCHAR* shaderFilePath, LPCSTR entryPoint, LPCSTR target, UINT flags, const D3D_SHADER_MACRO *defines);

	ID3DBlob* mVertexShader;
	ID3DBlob* mPixelShader;
	ID3DBlob* mHullShader;
	ID3DBlob* mDomainShader;

	bool mUsesTessellation;
	bool mHasPixelShader;

	D3D12_INPUT_ELEMENT_DESC mInputElementDescs[6];
	D3D12_INPUT_LAYOUT_DESC mInputLayoutDesc; //TDA: eventually need to break this out
};