#include "Render/RayTrace/RayTraceManager.h"
#include "Render/Mesh/Mesh.h"
#include "Util/String/StringConverter.h"
#include "Asset/Manifest/ManifestLoader.h"

//TDA: Implement update support
RayTraceManager::RayTraceManager(Direct3DManager *direct3DManager, RootSignatureManager *rootSignatureManager)
{
    mDirect3DManager = direct3DManager;

    mAccelerationStructure = new RayTraceAccelerationStructure(mDirect3DManager, false);
    mShouldBuildAccelerationStructure = false;
    mIsStructureReady = false;
    mStructureCreationFence = 0;

    LoadRayTraceShaders(rootSignatureManager);
}

RayTraceManager::~RayTraceManager()
{
    Application::Assert(mStructureCreationFence == 0); //if the structure is in mid-flight build, nuking it would blow up

    if (mAccelerationStructure)
    {
        delete mAccelerationStructure;
    }

    if (mVertexBuffer)
    {
        delete mVertexBuffer;
    }

    if (mIndexBuffer)
    {
        delete mIndexBuffer;
    }
}

void RayTraceManager::AddMeshToAccelerationStructure(Mesh *meshToAdd)
{
    mMeshesForRayAcceleration.Add(meshToAdd);
}

void RayTraceManager::QueueRayTraceAccelerationStructureCreation()
{
    mShouldBuildAccelerationStructure = true;

    //create simple triangle
    mVertices[0].Position = Vector3( 0.0f, -0.7f, 1.0f);
    mVertices[1].Position = Vector3(-0.7f,  0.7f, 1.0f);
    mVertices[2].Position = Vector3( 0.7f,  0.7f, 1.0f);

    mIndices[0] = 0;
    mIndices[1] = 1;
    mIndices[2] = 2;

    mVertexBuffer = mDirect3DManager->GetContextManager()->CreateVertexBuffer(mVertices, sizeof(RayTraceAccelerationStructure::RTXVertex), 3 * sizeof(RayTraceAccelerationStructure::RTXVertex));
    mIndexBuffer = mDirect3DManager->GetContextManager()->CreateIndexBuffer(mIndices, sizeof(uint32) * 3);
}

void RayTraceManager::Update()
{
    //TDA: turn this back on when meshes are supported
    if (/*mMeshesForRayAcceleration.CurrentSize() == 0 ||*/ mIsStructureReady)
    {
        //nothing to do if there's no meshes or we've already built it (no rebuild support here yet)
        return;
    }

    if (mStructureCreationFence > 0)
    {
        uint64 currentFence = mDirect3DManager->GetContextManager()->GetQueueManager()->GetGraphicsQueue()->PollCurrentFenceValue();
        if (currentFence >= mStructureCreationFence)
        {
            mStructureCreationFence = 0;
            mIsStructureReady = true;
            return;
        }
    }

    if (mShouldBuildAccelerationStructure)
    {
        bool readyForCreation = true;

        //TDA: make this actually support meshes
        const uint32 numMeshes = mMeshesForRayAcceleration.CurrentSize();
        for (uint32 i = 0; i < numMeshes; i++)
        {
            readyForCreation &= mMeshesForRayAcceleration[i]->IsReady();
        }

        readyForCreation &= mVertexBuffer->GetIsReady();
        readyForCreation &= mIndexBuffer->GetIsReady();

        if (readyForCreation)
        {
            mAccelerationStructure->ClearDescs();

            for (uint32 i = 0; i < numMeshes; i++)
            {
                mAccelerationStructure->AddMesh(mVertexBuffer, mIndexBuffer, 3, 3);
            }

            D3DXMATRIX structureMatrix;
            D3DXMatrixIdentity(&structureMatrix);

            mAccelerationStructure->BuildBottomLevelStructure(false);
            mAccelerationStructure->AddBottomLevelInstance(structureMatrix, 0, 0);
            mAccelerationStructure->BuildTopLevelStructure(false);

            Direct3DContextManager *contextManager = mDirect3DManager->GetContextManager();
            mStructureCreationFence = contextManager->GetRayTraceContext()->Flush(contextManager->GetQueueManager(), true, false);
            mShouldBuildAccelerationStructure = false;
        }
    }
}

void RayTraceManager::LoadRayTraceShaders(RootSignatureManager *rootSignatureManager)
{
    //TDA: clean up memory use
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcSupport.Initialize());
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcSupport.CreateInstance(CLSID_DxcCompiler, &mRayTraceShaderBuilder.DxcCompiler));
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcSupport.CreateInstance(CLSID_DxcLibrary, &mRayTraceShaderBuilder.DxcLibrary));
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcLibrary->CreateIncludeHandler(&mRayTraceShaderBuilder.DxcIncludeHandler));

    mRayTraceShaderInfos.Add(CompileRayShader(L"../Eden/data/HLSL/RayTrace/RayTestGen.hlsl", L"RayGen"));
    mRayTraceShaderInfos.Add(CompileRayShader(L"../Eden/data/HLSL/RayTrace/RayTestMiss.hlsl", L"Miss"));
    mRayTraceShaderInfos.Add(CompileRayShader(L"../Eden/data/HLSL/RayTrace/RayTestClosestHit.hlsl", L"ClosestHit"));

    RootSignatureAssociation *newRootSignatureAssociation = new RootSignatureAssociation();
    newRootSignatureAssociation->Symbols.Add(L"RayGen");
    newRootSignatureAssociation->SymbolPointers.Add(newRootSignatureAssociation->Symbols[0].c_str());
    newRootSignatureAssociation->RootSignature = rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Test_Generation).RootSignature;
    mRootSignatureAssociations.Add(newRootSignatureAssociation);

    newRootSignatureAssociation = new RootSignatureAssociation();
    newRootSignatureAssociation->Symbols.Add(L"Miss");
    newRootSignatureAssociation->SymbolPointers.Add(newRootSignatureAssociation->Symbols[0].c_str());
    newRootSignatureAssociation->RootSignature = rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Test_Hit_And_Miss).RootSignature;
    mRootSignatureAssociations.Add(newRootSignatureAssociation);

    newRootSignatureAssociation = new RootSignatureAssociation();
    newRootSignatureAssociation->Symbols.Add(L"HitGroup");
    newRootSignatureAssociation->SymbolPointers.Add(newRootSignatureAssociation->Symbols[0].c_str());
    newRootSignatureAssociation->RootSignature = rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Test_Hit_And_Miss).RootSignature;
    mRootSignatureAssociations.Add(newRootSignatureAssociation);

    mEmptyGlobalRootSignature = rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Empty_Global);
    mEmptyLocalRootSignature = rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Empty_Local);

    BuildPipelineResources();
}

RayTraceManager::RayTraceShaderInfo *RayTraceManager::CompileRayShader(WCHAR *fileName, WCHAR *symbolName)
{
    std::ifstream rayShaderFile(fileName);
    if (!rayShaderFile.is_open())
    {
        Application::Assert(false);
    }

    std::stringstream strStream;
    strStream << rayShaderFile.rdbuf();
    std::string rayShaderString = strStream.str();

    IDxcBlobEncoding *rayShaderBlobEncoding;
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)rayShaderString.c_str(), (uint32_t)rayShaderString.size(), 0, &rayShaderBlobEncoding));

    IDxcOperationResult* compilationResult;
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcCompiler->Compile(rayShaderBlobEncoding, fileName, L"", L"lib_6_3", nullptr, 0, nullptr, 0,
        mRayTraceShaderBuilder.DxcIncludeHandler, &compilationResult));

    HRESULT resultCode;
    Direct3DUtils::ThrowIfHRESULTFailed(compilationResult->GetStatus(&resultCode));
    if (FAILED(resultCode))
    {
        IDxcBlobEncoding* pError;
        if (FAILED(compilationResult->GetErrorBuffer(&pError)))
        {
            Direct3DUtils::ThrowLogicError("Failed to get shader compiler error.");
        }

        Direct3DUtils::ThrowLogicError("Shader compilation error.");
    }

    IDxcBlob* shaderBlob;
    Direct3DUtils::ThrowIfHRESULTFailed(compilationResult->GetResult(&shaderBlob));

    RayTraceShaderInfo *rayShaderInfo = new RayTraceShaderInfo();
    rayShaderInfo->RayTraceShader = shaderBlob;
    rayShaderInfo->ExportedSymbols.Add(symbolName);
    
    D3D12_EXPORT_DESC exportDesc;
    exportDesc.Name = rayShaderInfo->ExportedSymbols[0].c_str();
    exportDesc.ExportToRename = NULL;
    exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;
    rayShaderInfo->Exports.Add(exportDesc);

    D3D12_DXIL_LIBRARY_DESC libraryDesc;
    libraryDesc.DXILLibrary.BytecodeLength = shaderBlob->GetBufferSize();
    libraryDesc.DXILLibrary.pShaderBytecode = shaderBlob->GetBufferPointer();
    libraryDesc.NumExports = 1;
    libraryDesc.pExports = &rayShaderInfo->Exports[0];
    
    rayShaderInfo->LibraryDesc = libraryDesc;

    return rayShaderInfo;
}

void RayTraceManager::BuildPipelineResources()
{
    //TDA: memory cleanup

    uint32 subObjectCount =
        3 + //shader count
        1 + //hit group
        1 + //shader config
        1 + //shader payload
        2 * 3 +//2 * number of root signature associations
        2 + //global and local root signatures
        1; //final pipeline object

    D3D12_STATE_SUBOBJECT *stateObjects = new D3D12_STATE_SUBOBJECT[subObjectCount];
    uint32 stateObjectIndex = 0;

    //3
    for (uint32 i = 0; i < mRayTraceShaderInfos.CurrentSize(); i++)
    {
        const RayTraceShaderInfo *rayTraceShaderInfo = mRayTraceShaderInfos[i];

        D3D12_STATE_SUBOBJECT librarySubObject = {};
        librarySubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        librarySubObject.pDesc = &rayTraceShaderInfo->LibraryDesc;

        stateObjects[stateObjectIndex++] = librarySubObject;
    }

    D3D12_HIT_GROUP_DESC hitGroupDesc;
    hitGroupDesc.HitGroupExport = L"HitGroup";
    hitGroupDesc.ClosestHitShaderImport = L"ClosestHit";
    hitGroupDesc.AnyHitShaderImport = NULL;
    hitGroupDesc.IntersectionShaderImport = NULL;

    //1
    D3D12_STATE_SUBOBJECT hitGroupSubObject = {};
    hitGroupSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    hitGroupSubObject.pDesc = &hitGroupDesc;
    stateObjects[stateObjectIndex++] = hitGroupSubObject;

    D3D12_RAYTRACING_SHADER_CONFIG shaderDesc = {};
    shaderDesc.MaxPayloadSizeInBytes = 4 * sizeof(float); //rgb + distance
    shaderDesc.MaxAttributeSizeInBytes = 2 * sizeof(float); //barycentrics

    //1
    D3D12_STATE_SUBOBJECT shaderConfigSubObject = {};
    shaderConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    shaderConfigSubObject.pDesc = &shaderDesc;
    stateObjects[stateObjectIndex++] = shaderConfigSubObject;

    DynamicArray<std::wstring> exportedSymbols;
    DynamicArray<LPCWSTR> exportedSymbolPointers;
    exportedSymbols.Add(L"RayGen");
    exportedSymbols.Add(L"Miss");
    exportedSymbols.Add(L"HitGroup");

    exportedSymbolPointers.Add(exportedSymbols[0].c_str());
    exportedSymbolPointers.Add(exportedSymbols[1].c_str());
    exportedSymbolPointers.Add(exportedSymbols[2].c_str());

    const WCHAR** shaderExports = exportedSymbolPointers.GetInnerArrayNonConst();

    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
    shaderPayloadAssociation.NumExports = static_cast<UINT>(exportedSymbols.CurrentSize());
    shaderPayloadAssociation.pExports = shaderExports;
    shaderPayloadAssociation.pSubobjectToAssociate = &stateObjects[(stateObjectIndex - 1)];

    //1
    D3D12_STATE_SUBOBJECT shaderPayloadAssociationObject = {};
    shaderPayloadAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    shaderPayloadAssociationObject.pDesc = &shaderPayloadAssociation;
    stateObjects[stateObjectIndex++] = shaderPayloadAssociationObject;

    //2 * 3
    for (uint32 i = 0; i < mRootSignatureAssociations.CurrentSize(); i++)
    {
        // Add a subobject to declare the root signature
        D3D12_STATE_SUBOBJECT rootSigObject = {};
        rootSigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        rootSigObject.pDesc = &mRootSignatureAssociations[i]->RootSignature;

        stateObjects[stateObjectIndex++] = rootSigObject;

        // Add a subobject for the association between the exported shader symbols and the root
        // signature
        mRootSignatureAssociations[i]->Association.NumExports = static_cast<UINT>(mRootSignatureAssociations[i]->SymbolPointers.CurrentSize());
        mRootSignatureAssociations[i]->Association.pExports = mRootSignatureAssociations[i]->SymbolPointers.GetInnerArrayNonConst();
        mRootSignatureAssociations[i]->Association.pSubobjectToAssociate = &stateObjects[(stateObjectIndex - 1)];

        D3D12_STATE_SUBOBJECT rootSigAssociationObject = {};
        rootSigAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        rootSigAssociationObject.pDesc = &mRootSignatureAssociations[i]->Association;

        stateObjects[stateObjectIndex++] = rootSigAssociationObject;
    }

    //2
    D3D12_STATE_SUBOBJECT globalRootSig;
    globalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
    ID3D12RootSignature* dgSig = mEmptyGlobalRootSignature.RootSignature;
    globalRootSig.pDesc = &dgSig;

    stateObjects[stateObjectIndex++] = globalRootSig;

    D3D12_STATE_SUBOBJECT localRootSig;
    localRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    ID3D12RootSignature* dlSig = mEmptyLocalRootSignature.RootSignature;
    localRootSig.pDesc = &dlSig;
    stateObjects[stateObjectIndex++] = localRootSig;

    //1
    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
    pipelineConfig.MaxTraceRecursionDepth = 1;

    D3D12_STATE_SUBOBJECT pipelineConfigObject = {};
    pipelineConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    pipelineConfigObject.pDesc = &pipelineConfig;
    stateObjects[stateObjectIndex++] = pipelineConfigObject;

    D3D12_STATE_OBJECT_DESC pipelineDesc = {};
    pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    pipelineDesc.NumSubobjects = stateObjectIndex; // static_cast<UINT>(subobjects.size());
    pipelineDesc.pSubobjects = stateObjects;

    Direct3DUtils::ThrowIfHRESULTFailed(mDirect3DManager->GetRayTraceDevice()->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&mRayTraceStateObject)));
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceStateObject->QueryInterface(IID_PPV_ARGS(&mRayTraceStateObjectProperties)));

    mRayTraceRenderTarget = mDirect3DManager->GetContextManager()->CreateRenderTarget(mDirect3DManager->)
}