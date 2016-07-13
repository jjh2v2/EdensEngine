#pragma once
#include "Render/Shader/ShaderPipelineDefinition.h"

class Shader
{
public:
	Shader(ID3D12Device* device, ShaderPipelineDefinition &initData, ShaderPipelineRenderState &renderState,
		ShaderPipelineTargetState &targetState, const D3D_SHADER_MACRO *defines);
	~Shader();

	void ReadShaderByteCodeFromFile(const std::string &shaderOutputName, void *&data, uint32 &dataLength);
	void OutputShaderByteCodeToFile(const std::string &shaderOutputName, void* data, uint32 dataLength);
	ID3DBlob *GetShaderCode(const std::string &compiledShaderFileLocation, WCHAR* shaderFilePath, LPCSTR entryPoint, LPCSTR target, UINT flags, const D3D_SHADER_MACRO *defines);

private:
	bool Initialize(ID3D12Device* device, ShaderPipelineDefinition &initData, ShaderPipelineRenderState &renderState, 
		ShaderPipelineTargetState &targetState, const D3D_SHADER_MACRO *defines);

	bool mUsesTessellation;
	bool mHasPixelShader;

	ID3D12RootSignature *mRootSignature;
	ID3D12PipelineState *mPipelineState;
};