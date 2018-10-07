#pragma once
#include "Render/RayTrace/RayTraceAccelerationStructure.h"
#include "Render/DirectX/DXRExperimental/dxcapi.h"
#include "Render/DirectX/DXRExperimental/dxcapi.use.h"

//using nvidia helpers for the clumsier part of the ray pipeline
#include "Render/DirectX/DXRExperimental/Helpers/RaytracingPipelineGenerator.h"
#include "Render/DirectX/DXRExperimental/Helpers/ShaderBindingTableGenerator.h"

class Camera;

class RayTraceManager
{
public:
    RayTraceManager(Direct3DManager *direct3DManager, RootSignatureManager *rootSignatureManager);
    ~RayTraceManager();

    void QueueRayTraceAccelerationStructureCreation();
    void Update(Camera *camera);
    bool GetIsStructureReady() { return mIsStructureReady; }
    RenderTarget *GetRenderTarget() { return mRayTraceRenderTarget; }

private:
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
    ID3D12StateObject *mRayTraceStateObject;
    ID3D12StateObjectProperties *mRayTraceStateObjectProperties;
    RenderTarget *mRayTraceRenderTarget;
    DescriptorHeap *mRayTraceHeap;
    ConstantBuffer *mCameraBuffers[FRAME_BUFFER_COUNT];

    nv_helpers_dx12::ShaderBindingTableGenerator mShaderBindingTableHelper;
    RayTraceBuffer *mShaderBindingTableStorage;

    IDxcBlob *mRayGenShader;
    IDxcBlob *mMissShader;
    IDxcBlob *mHitShader;
    ID3D12RootSignature *mRayGenSignature;
    ID3D12RootSignature *mHitSignature;
    ID3D12RootSignature *mMissSignature;
};