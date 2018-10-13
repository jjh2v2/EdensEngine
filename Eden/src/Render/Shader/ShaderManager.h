#pragma once
#include "Render/Shader/ShaderTechnique.h"
#include "Core/Containers/DynamicArray.h"
#include "Asset/Manifest/ManifestLoader.h"
#include "Render/Shader/RootSignature/RootSignatureManager.h"
#include <map>

class ShaderManager
{
public:
	ShaderManager(ID3D12Device *device);
	~ShaderManager();
	
	ShaderPSO *GetShader(const std::string &shaderName, const ShaderPipelinePermutation &permutation);
    RootSignatureManager *GetRootSignatureManager() { return mRootSignatureManager; }

private:
	struct ShaderTechniqueInfo
	{
		ShaderTechnique *Technique;
		RootSignatureType SignatureType;
	};

	void LoadAllShaders(ID3D12Device *device);
	ShaderTechniqueInfo LoadShader(ID3D12Device *device, const char *fileName);
	void PreloadExpectedPermutations();
    
	ID3D12Device *mDevice;

	std::map<std::string, ShaderTechniqueInfo> mShaderTechniqueLookup;
	DynamicArray<ShaderTechnique*> mShaderTechniques;
	ManifestLoader mManifestLoader;

	RootSignatureManager *mRootSignatureManager;
};