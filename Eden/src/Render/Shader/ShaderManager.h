#pragma once
#include "Render/Shader/ShaderTechnique.h"
#include "Core/Containers/DynamicArray.h"
#include <map>
#include "Asset/Manifest/ManifestLoader.h"
#include "Render/Shader/RootSignature/RootSignatureManager.h"

#include "Render/DirectX/DXRExperimental/dxcapi.h"
#include "Render/DirectX/DXRExperimental/dxcapi.use.h"

class ShaderManager
{
public:
	ShaderManager(ID3D12Device *device);
	~ShaderManager();
	
	ShaderPSO *GetShader(std::string shaderName, const ShaderPipelinePermutation &permutation);

private:
	struct ShaderTechniqueInfo
	{
		ShaderTechnique *Technique;
		RootSignatureType SignatureType;
	};

    struct RayTraceShaderBuilder
    {
        dxc::DxcDllSupport DxcSupport;
        IDxcCompiler *DxcCompiler;
        IDxcLibrary *DxcLibrary;
        IDxcIncludeHandler *DxcIncludeHandler;
    };

	void LoadAllShaders(ID3D12Device *device);
	ShaderTechniqueInfo LoadShader(ID3D12Device *device, const char *fileName);
	void PreloadExpectedPermutations();

    void LoadRayTraceShaders();
    IDxcBlob *CompileRayShader(WCHAR *fileName);
    IDxcBlob* CompileShaderLibrary(LPCWSTR fileName);

	ID3D12Device *mDevice;

	std::map<std::string, ShaderTechniqueInfo> mShaderTechniqueLookup;
	DynamicArray<ShaderTechnique*> mShaderTechniques;
	ManifestLoader mManifestLoader;

	RootSignatureManager *mRootSignatureManager;

    RayTraceShaderBuilder mRayTraceShaderBuilder;
    IDxcBlob *mRayGenerationShader;
    IDxcBlob *mRayClosestHitShader;
    IDxcBlob *mRayMissShader;
};