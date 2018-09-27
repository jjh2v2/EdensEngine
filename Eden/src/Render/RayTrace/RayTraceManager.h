#pragma once
#include "Render/RayTrace/RayTraceAccelerationStructure.h"
#include "Render/DirectX/DXRExperimental/dxcapi.h"
#include "Render/DirectX/DXRExperimental/dxcapi.use.h"

class RayTraceManager
{
public:
    RayTraceManager(Direct3DManager *direct3DManager, RootSignatureManager *rootSignatureManager);
    ~RayTraceManager();

    void AddMeshToAccelerationStructure(Mesh *meshToAdd);
    void QueueRayTraceAccelerationStructureCreation();
    void Update();
    bool GetIsStructureReady() { return mIsStructureReady; }

private:
    struct RayTraceShaderBuilder
    {
        dxc::DxcDllSupport DxcSupport;
        IDxcCompiler *DxcCompiler;
        IDxcLibrary *DxcLibrary;
        IDxcIncludeHandler *DxcIncludeHandler;
    };

    struct RayTraceShaderInfo
    {
        IDxcBlob *RayTraceShader;
        D3D12_DXIL_LIBRARY_DESC LibraryDesc;
        DynamicArray<std::wstring> ExportedSymbols;
        DynamicArray<D3D12_EXPORT_DESC> Exports;
    };

    struct RootSignatureAssociation
    {
        ID3D12RootSignature *RootSignature;
        DynamicArray<std::wstring> Symbols;
        DynamicArray<LPCWSTR> SymbolPointers;
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION Association;
    };

    void LoadRayTraceShaders(RootSignatureManager *rootSignatureManager);
    RayTraceShaderInfo *CompileRayShader(WCHAR *fileName, WCHAR *symbolName);
    void BuildPipelineResources();

    RayTraceAccelerationStructure::RTXVertex mVertices[3];
    uint32 mIndices[3];
    VertexBuffer *mVertexBuffer;
    IndexBuffer *mIndexBuffer;

    Direct3DManager *mDirect3DManager;

    bool mShouldBuildAccelerationStructure;
    bool mIsStructureReady;
    uint64 mStructureCreationFence;
    DynamicArray<Mesh*> mMeshesForRayAcceleration;
    RayTraceAccelerationStructure *mAccelerationStructure;

    RayTraceShaderBuilder mRayTraceShaderBuilder;

    DynamicArray<RayTraceShaderInfo*> mRayTraceShaderInfos;
    DynamicArray<RootSignatureAssociation*> mRootSignatureAssociations;
    RootSignatureInfo mEmptyGlobalRootSignature;
    RootSignatureInfo mEmptyLocalRootSignature;

    ID3D12StateObjectPrototype *mRayTraceStateObject;
    ID3D12StateObjectPropertiesPrototype *mRayTraceStateObjectProperties;
    RenderTarget *mRayTraceRenderTarget;
};