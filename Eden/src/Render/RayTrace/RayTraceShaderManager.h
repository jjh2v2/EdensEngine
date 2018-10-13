#pragma once
#include "Render/Buffer/GPUResource.h"
#include "Render/DirectX/DXR/dxcapi.h"
#include "Render/DirectX/DXR/dxcapi.use.h"
#include "Asset/Manifest/ManifestLoader.h"
#include <map>

class Direct3DManager;
class RootSignatureManager;

class RayTraceShaderManager
{
public:
    RayTraceShaderManager(Direct3DManager *direct3DManager, RootSignatureManager *rootSignatureManager);
    ~RayTraceShaderManager();

    struct RayTracePSO
    {
        IDxcBlob *GenShader;
        IDxcBlob *MissShader;
        IDxcBlob *ClosestHitShader;
        RayTraceBuffer *GenShaderTable;
        RayTraceBuffer *MissShaderTable;
        RayTraceBuffer *HitGroupShaderTable;
        ID3D12StateObject *RayTraceStateObject;
        ID3D12StateObjectProperties *RayTraceStateObjectProperties;
        ID3D12RootSignature *GlobalRootSignature;
    };

    RayTracePSO *GetRayTracePipeline(const std::string &pipelineName);

private:
    struct DXCInterface
    {
        dxc::DxcDllSupport DxcSupport;
        IDxcCompiler *DxcCompiler;
        IDxcLibrary *DxcLibrary;
        IDxcIncludeHandler *DxcIncludeHandler;
    };

    void LoadAllPipelines();
    RayTracePSO *LoadPipeline(const char *fileName);
    IDxcBlob *CompileRayShader(WCHAR *fileName);

    Direct3DManager *mDirect3DManager;
    RootSignatureManager *mRootSignatureManager;

    DXCInterface mDXC;
    ManifestLoader mManifestLoader;
    std::map<std::string, RayTracePSO*> mRayTracePipelineLookup;
    DynamicArray<RayTracePSO*> mRayTracePipelines;
};