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
	Shader(ID3D12Device* device, ShaderPipelineDefinition &initData, ShaderPipelineRenderState &renderState,
		ShaderPipelineTargetState &targetState);
	~Shader();

private:
	bool Initialize(ID3D12Device* device, ShaderPipelineDefinition &initData, ShaderPipelineRenderState &renderState, 
		ShaderPipelineTargetState &targetState);
	void ReadShaderByteCodeFromFile(const std::string &shaderOutputName, void *&data, uint32 &dataLength);
	void OutputShaderByteCodeToFile(const std::string &shaderOutputName, void* data, uint32 dataLength);
	ID3DBlob *GetShaderCode(const std::string &compiledShaderFileLocation, WCHAR* shaderFilePath, LPCSTR entryPoint, LPCSTR target, UINT flags, const D3D_SHADER_MACRO *defines);

	bool mUsesTessellation;
	bool mHasPixelShader;

	ID3D12RootSignature *mRootSignature;
	ID3D12PipelineState *mPipelineState;
};