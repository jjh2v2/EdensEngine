#pragma once
#include "Render/Shader/ShaderTechnique.h"
#include "Core/Containers/DynamicArray.h"
#include <map>
#include "Asset/Manifest/ManifestLoader.h"
#include "Render/Shader/RootSignature/RootSignatureManager.h"

class ShaderManager
{
public:
	ShaderManager(ID3D12Device *device);
	~ShaderManager();
	
private:
	void LoadAllShaders(ID3D12Device *device);
	ShaderTechnique *LoadShader(ID3D12Device *device, const char *fileName);

	std::map<std::string, ShaderTechnique*> mShaderTechniqueLookup;
	DynamicArray<ShaderTechnique*> mShaderTechniques;
	ManifestLoader mManifestLoader;

	RootSignatureManager *mRootSignatureManager;
};