#pragma once
#include "Render/RayTrace/RayTraceAccelerationStructure.h"
#include "Render/DirectX/DXRExperimental/dxcapi.h"
#include "Render/DirectX/DXRExperimental/dxcapi.use.h"

//using nvidia helpers for the clumsier part of the ray pipeline
#include "Render/DirectX/DXRExperimental/Helpers/RaytracingPipelineGenerator.h"
#include "Render/DirectX/DXRExperimental/Helpers/ShaderBindingTableGenerator.h"

class RayTraceManager
{
public:
    RayTraceManager(Direct3DManager *direct3DManager, RootSignatureManager *rootSignatureManager);
    ~RayTraceManager();

    void QueueRayTraceAccelerationStructureCreation();
    void Update();
    bool GetIsStructureReady() { return mIsStructureReady; }
    RenderTarget *GetRenderTarget() { return mRayTraceRenderTarget; }

private:
    struct AccelerationStructureBuffers
    {
        ID3D12Resource *pScratch;      // Scratch memory for AS builder
        ID3D12Resource *pResult;       // Where the AS is
        ID3D12Resource *pInstanceDesc; // Hold the matrices of the instances
    };

    struct RayTraceShaderBuilder
    {
        dxc::DxcDllSupport DxcSupport;
        IDxcCompiler *DxcCompiler;
        IDxcLibrary *DxcLibrary;
        IDxcIncludeHandler *DxcIncludeHandler;
    };

    void LoadRayTraceShaders(RootSignatureManager *rootSignatureManager);
    IDxcBlob *CompileRayShader(WCHAR *fileName);
    void BuildHeap();
    void BuildShaderBindingTable();
    void DispatchRayTrace();
    void CreateAccelerationStructures();

    RayTraceAccelerationStructure::RTXVertex mVertices[3];
    VertexBuffer *mVertexBuffer;
    Direct3DManager *mDirect3DManager;

    bool mShouldBuildAccelerationStructure;
    bool mIsStructureReady;

    RayTraceShaderBuilder mRayTraceShaderBuilder;
    //RootSignatureInfo mEmptyGlobalRootSignature;
    //RootSignatureInfo mEmptyLocalRootSignature;

    RayTraceAccelerationStructure *mAccelerationStructure;
    ID3D12StateObjectPrototype *mRayTraceStateObject;
    ID3D12StateObjectPropertiesPrototype *mRayTraceStateObjectProperties;
    RenderTarget *mRayTraceRenderTarget;
    DescriptorHeap *mRayTraceHeap;

    nv_helpers_dx12::ShaderBindingTableGenerator mShaderBindingTableHelper;
    RayTraceBuffer *mShaderBindingTableStorage;

    IDxcBlob *mRayGenShader;
    IDxcBlob *mMissShader;
    IDxcBlob *mHitShader;
    ID3D12RootSignature *mRayGenSignature;
    ID3D12RootSignature *mHitSignature;
    ID3D12RootSignature *mMissSignature;
};