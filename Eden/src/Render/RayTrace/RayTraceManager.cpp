#include "Render/RayTrace/RayTraceManager.h"
#include "Render/Mesh/Mesh.h"
#include "Util/String/StringConverter.h"
#include "Asset/Manifest/ManifestLoader.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"
#include "Camera/Camera.h"

#include "Render/DirectX/DXRExperimental/Helpers/D3D12RaytracingHelpers.hpp"

//TDA: Implement update support
RayTraceManager::RayTraceManager(Direct3DManager *direct3DManager, RootSignatureManager *rootSignatureManager)
{
    mDirect3DManager = direct3DManager;

    mAccelerationStructure = new RayTraceAccelerationStructure(mDirect3DManager, false);
    mShouldBuildAccelerationStructure = false;
    mIsStructureReady = false;
    //mStructureCreationFence = 0;

    Vector2 screenSize = mDirect3DManager->GetScreenSize();
    mRayTraceRenderTarget = mDirect3DManager->GetContextManager()->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R8G8B8A8_UNORM, true, 1, 1, 0);

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mCameraBuffers[i] = mDirect3DManager->GetContextManager()->CreateConstantBuffer(sizeof(CameraRayTraceBuffer));
    }

    LoadRayTraceShaders(rootSignatureManager);
    mRootSignatureManager = rootSignatureManager;
}

RayTraceManager::~RayTraceManager()
{
    if (mVertexBuffer)
    {
        delete mVertexBuffer;
    }

    if (mAccelerationStructure)
    {
        delete mAccelerationStructure;
    }
}

void RayTraceManager::QueueRayTraceAccelerationStructureCreation()
{
    mShouldBuildAccelerationStructure = true;

    //create simple triangle
    mVertices[0].Position = Vector3( 0.0f, 0.25f, 0.0f);
    mVertices[1].Position = Vector3( 0.25f, -0.25f, 0.0f);
    mVertices[2].Position = Vector3( -0.25f, -0.25f, 0.0f);

    mVertexBuffer = mDirect3DManager->GetContextManager()->CreateVertexBuffer(mVertices, sizeof(RayTraceAccelerationStructure::RTXVertex), 3 * sizeof(RayTraceAccelerationStructure::RTXVertex));
}

void RayTraceManager::Update(Camera *camera)
{
    D3DXMATRIX cameraView = camera->GetViewMatrix();
    D3DXMATRIX cameraViewInv;
    D3DXMatrixInverse(&cameraViewInv, NULL, &cameraView);

    D3DXMATRIX cameraProj = camera->GetProjectionMatrix();
    D3DXMATRIX cameraProjInv;
    D3DXMatrixInverse(&cameraProjInv, NULL, &cameraProj);

    CameraRayTraceBuffer cameraBuffer;
    cameraBuffer.viewMatrix = cameraView;
    cameraBuffer.viewInvMatrix = cameraViewInv;
    cameraBuffer.projectionMatrix = camera->GetProjectionMatrix();
    cameraBuffer.projectionInvMatrix = cameraProjInv;
    mCameraBuffers[mDirect3DManager->GetFrameIndex()]->SetConstantBufferData(&cameraBuffer, sizeof(CameraRayTraceBuffer));

    if (mIsStructureReady)
    {
        //nothing to do if there's no meshes or we've already built it (no rebuild support here yet)
        DispatchRayTrace();
        return;
    }

    if (mShouldBuildAccelerationStructure)
    {
        bool readyForCreation = true;

        readyForCreation &= mVertexBuffer->GetIsReady();

        if (readyForCreation)
        {
            CreateAccelerationStructures();
            BuildHeap();
            BuildShaderBindingTable();
            DispatchRayTrace();
            mIsStructureReady = true;
        }
    }
}

void RayTraceManager::LoadRayTraceShaders(RootSignatureManager *rootSignatureManager)
{
    //TDA: clean up memory use of everything in this file
     Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcSupport.Initialize());
     Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcSupport.CreateInstance(CLSID_DxcCompiler, &mRayTraceShaderBuilder.DxcCompiler));
     Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcSupport.CreateInstance(CLSID_DxcLibrary, &mRayTraceShaderBuilder.DxcLibrary));
     Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcLibrary->CreateIncludeHandler(&mRayTraceShaderBuilder.DxcIncludeHandler));

    mRayGenSignature = rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Test_Hit_And_Miss).RootSignature;
    mHitSignature = rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Test_Hit_And_Miss).RootSignature;
    mMissSignature = rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Test_Hit_And_Miss).RootSignature;

    mRayGenShader = CompileRayShader(L"../Eden/data/HLSL/RayTrace/RayGen.hlsl");
    mMissShader = CompileRayShader(L"../Eden/data/HLSL/RayTrace/Miss.hlsl");
    mHitShader = CompileRayShader(L"../Eden/data/HLSL/RayTrace/Hit.hlsl");

    CD3D12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    
    CD3D12_DXIL_LIBRARY_SUBOBJECT *librarySubObject = raytracingPipeline.CreateSubobject<CD3D12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE libraryByteCode1 = CD3DX12_SHADER_BYTECODE(mRayGenShader->GetBufferPointer(), mRayGenShader->GetBufferSize());
    librarySubObject->SetDXILLibrary(&libraryByteCode1);
    librarySubObject->DefineExport(L"RayGen");


    CD3D12_DXIL_LIBRARY_SUBOBJECT *librarySubObject2 = raytracingPipeline.CreateSubobject<CD3D12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE libraryByteCode2 = CD3DX12_SHADER_BYTECODE(mMissShader->GetBufferPointer(), mMissShader->GetBufferSize());
    librarySubObject2->SetDXILLibrary(&libraryByteCode2);
    librarySubObject2->DefineExport(L"Miss");


    CD3D12_DXIL_LIBRARY_SUBOBJECT *librarySubObject3 = raytracingPipeline.CreateSubobject<CD3D12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE libraryByteCode3 = CD3DX12_SHADER_BYTECODE(mHitShader->GetBufferPointer(), mHitShader->GetBufferSize());
    librarySubObject3->SetDXILLibrary(&libraryByteCode3);
    librarySubObject3->DefineExport(L"ClosestHit");
    
    
    CD3D12_HIT_GROUP_SUBOBJECT *hitGroupSubObject = raytracingPipeline.CreateSubobject<CD3D12_HIT_GROUP_SUBOBJECT>();
    hitGroupSubObject->SetClosestHitShaderImport(L"ClosestHit");
    hitGroupSubObject->SetHitGroupExport(L"HitGroup");
    hitGroupSubObject->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    CD3D12_RAYTRACING_SHADER_CONFIG_SUBOBJECT *shaderConfig = raytracingPipeline.CreateSubobject<CD3D12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    UINT payloadSize = 4 * sizeof(float);   // float4 color
    UINT attributeSize = 2 * sizeof(float); // float2 barycentrics
    shaderConfig->Config(payloadSize, attributeSize);

    {
        auto localRootSignature = raytracingPipeline.CreateSubobject<CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
        localRootSignature->SetRootSignature(mRayGenSignature);
        // Shader association
        auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
        rootSignatureAssociation->AddExport(L"RayGen");
    }

    {
        auto localRootSignature = raytracingPipeline.CreateSubobject<CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
        localRootSignature->SetRootSignature(mMissSignature);
        // Shader association
        auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
        rootSignatureAssociation->AddExport(L"Miss");
    }

    {
        auto localRootSignature = raytracingPipeline.CreateSubobject<CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
        localRootSignature->SetRootSignature(mHitSignature);
        // Shader association
        auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
        rootSignatureAssociation->AddExport(L"HitGroup");
    }

    auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3D12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Global).RootSignature);

    auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3D12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    UINT maxRecursionDepth = 1; // ~ primary rays only. 
    pipelineConfig->Config(maxRecursionDepth);

    Direct3DUtils::ThrowIfHRESULTFailed(mDirect3DManager->GetRayTraceDevice()->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&mRayTraceStateObject)));
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceStateObject->QueryInterface(IID_PPV_ARGS(&mRayTraceStateObjectProperties)));
}

IDxcBlob *RayTraceManager::CompileRayShader(WCHAR *fileName)
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

        const char *pStart = pError ? (const char *)pError->GetBufferPointer() : NULL;
        Direct3DUtils::ThrowLogicError("Shader compilation error.");
    }

    IDxcBlob* shaderBlob;
    Direct3DUtils::ThrowIfHRESULTFailed(compilationResult->GetResult(&shaderBlob));

    return shaderBlob;
}

void RayTraceManager::CreateAccelerationStructures()
{
    D3DXMATRIX structureMatrix;
    D3DXMatrixIdentity(&structureMatrix);

    mAccelerationStructure->ClearDescs();
    mAccelerationStructure->AddMesh(mVertexBuffer, NULL, 3, 0);
    mAccelerationStructure->BuildBottomLevelStructure(false);
    mAccelerationStructure->AddBottomLevelInstance(structureMatrix, 0, 0);
    mAccelerationStructure->BuildTopLevelStructure(false);

    mDirect3DManager->GetContextManager()->GetRayTraceContext()->Flush(mDirect3DManager->GetContextManager()->GetQueueManager(), true);
}

void RayTraceManager::BuildShaderBindingTable()
{
    uint32 shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    {
        uint32 shaderRecordSize = MathHelper::AlignU32(shaderIdentifierSize/* + 8*/, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        mRayGenShaderTable = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(shaderRecordSize, D3D12_RESOURCE_STATE_GENERIC_READ, RayTraceBuffer::RayTraceBufferType_Shader_Binding_Table_Storage);
        const void* shaderId = mRayTraceStateObjectProperties->GetShaderIdentifier(L"RayGen");

        uint8_t *mappedData;
        mRayGenShaderTable->GetResource()->Map(0, NULL, reinterpret_cast<void**>(&mappedData));
        memcpy(mappedData, shaderId, shaderIdentifierSize);
        mRayGenShaderTable->GetResource()->Unmap(0, NULL);
    }

    {
        uint32 shaderRecordSize = MathHelper::AlignU32(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        mMissShaderTable = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(shaderRecordSize, D3D12_RESOURCE_STATE_GENERIC_READ, RayTraceBuffer::RayTraceBufferType_Shader_Binding_Table_Storage);
        const void* shaderId = mRayTraceStateObjectProperties->GetShaderIdentifier(L"Miss");

        uint8_t *mappedData;
        mMissShaderTable->GetResource()->Map(0, NULL, reinterpret_cast<void**>(&mappedData));
        memcpy(mappedData, shaderId, shaderIdentifierSize);
        mMissShaderTable->GetResource()->Unmap(0, NULL);
    }

    {
        uint32 shaderRecordSize = MathHelper::AlignU32(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        mHitGroupShaderTable = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(shaderRecordSize, D3D12_RESOURCE_STATE_GENERIC_READ, RayTraceBuffer::RayTraceBufferType_Shader_Binding_Table_Storage);
        const void* shaderId = mRayTraceStateObjectProperties->GetShaderIdentifier(L"HitGroup");

        uint8_t *mappedData;
        mHitGroupShaderTable->GetResource()->Map(0, NULL, reinterpret_cast<void**>(&mappedData));
        memcpy(mappedData, shaderId, shaderIdentifierSize);
        mHitGroupShaderTable->GetResource()->Unmap(0, NULL);
    }
}

void RayTraceManager::BuildHeap()
{
    mRayTraceHeap = new DescriptorHeap(mDirect3DManager->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RAY_TRACE_DESCRIPTOR_HEAP_SIZE, true);
    D3D12_CPU_DESCRIPTOR_HANDLE uavSrvHandle = mRayTraceHeap->GetHeapCPUStart();

    mDirect3DManager->GetDevice()->CopyDescriptorsSimple(1, uavSrvHandle, mRayTraceRenderTarget->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    uavSrvHandle.ptr += mRayTraceHeap->GetDescriptorSize();

    mDirect3DManager->GetDevice()->CopyDescriptorsSimple(1, uavSrvHandle, mAccelerationStructure->GetTopLevelResultBuffer()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RayTraceManager::DispatchRayTrace()
{
    D3D12_CPU_DESCRIPTOR_HANDLE uavSrvHandle = mRayTraceHeap->GetHeapCPUStart();
    //uavSrvHandle.ptr += mRayTraceHeap->GetDescriptorSize() * 2; //TDA: clean this up
    //mDirect3DManager->GetDevice()->CopyDescriptorsSimple(1, uavSrvHandle, mCameraBuffers[mDirect3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    RayTraceContext *rayTraceContext = mDirect3DManager->GetContextManager()->GetRayTraceContext();
    rayTraceContext->TransitionResource(mRayTraceRenderTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
    rayTraceContext->SetComputeRootSignature(mRootSignatureManager->GetRootSignature(RootSignatureType_Ray_Global).RootSignature);
    rayTraceContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mRayTraceHeap->GetHeap());
    rayTraceContext->SetComputeDescriptorTable(0, mRayTraceHeap->GetHeapGPUStart());
    rayTraceContext->SetRayPipelineState(mRayTraceStateObject);
    
    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
    dispatchDesc.RayGenerationShaderRecord.StartAddress = mRayGenShaderTable->GetGpuAddress();
    dispatchDesc.RayGenerationShaderRecord.SizeInBytes = mRayGenShaderTable->GetResource()->GetDesc().Width;
    dispatchDesc.MissShaderTable.StartAddress = mMissShaderTable->GetGpuAddress();
    dispatchDesc.MissShaderTable.SizeInBytes = mMissShaderTable->GetResource()->GetDesc().Width;
    dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes;
    dispatchDesc.HitGroupTable.StartAddress = mHitGroupShaderTable->GetGpuAddress();
    dispatchDesc.HitGroupTable.SizeInBytes = mHitGroupShaderTable->GetResource()->GetDesc().Width;
    dispatchDesc.HitGroupTable.StrideInBytes = dispatchDesc.HitGroupTable.SizeInBytes;
    dispatchDesc.Width = mRayTraceRenderTarget->GetWidth();
    dispatchDesc.Height = mRayTraceRenderTarget->GetHeight();
    dispatchDesc.Depth = 1;

    rayTraceContext->DispatchRays(dispatchDesc);
    rayTraceContext->Flush(mDirect3DManager->GetContextManager()->GetQueueManager(), true);
}