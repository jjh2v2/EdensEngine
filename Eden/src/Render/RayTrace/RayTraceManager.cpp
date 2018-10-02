#include "Render/RayTrace/RayTraceManager.h"
#include "Render/Mesh/Mesh.h"
#include "Util/String/StringConverter.h"
#include "Asset/Manifest/ManifestLoader.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"
#include "Camera/Camera.h"

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

    mRayGenSignature = rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Test_Generation).RootSignature;
    mHitSignature = rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Test_Hit_And_Miss).RootSignature;
    mMissSignature = rootSignatureManager->GetRootSignature(RootSignatureType_Ray_Test_Hit_And_Miss).RootSignature;

    mRayGenShader = CompileRayShader(L"../Eden/data/HLSL/RayTrace/RayGen.hlsl");
    mMissShader = CompileRayShader(L"../Eden/data/HLSL/RayTrace/Miss.hlsl");
    mHitShader = CompileRayShader(L"../Eden/data/HLSL/RayTrace/Hit.hlsl");

    nv_helpers_dx12::RayTracingPipelineGenerator pipeline(mDirect3DManager->GetDevice(), mDirect3DManager->GetRayTraceDevice());
    pipeline.AddLibrary(mRayGenShader, { L"RayGen" });
    pipeline.AddLibrary(mMissShader, { L"Miss" });
    pipeline.AddLibrary(mHitShader, { L"ClosestHit" });
    pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");
    pipeline.AddRootSignatureAssociation(mRayGenSignature, { L"RayGen" });
    pipeline.AddRootSignatureAssociation(mMissSignature, { L"Miss" });
    pipeline.AddRootSignatureAssociation(mHitSignature, { L"HitGroup" });
    pipeline.SetMaxPayloadSize(4 * sizeof(float));
    pipeline.SetMaxAttributeSize(2 * sizeof(float));
    pipeline.SetMaxRecursionDepth(1);

    mRayTraceStateObject = pipeline.Generate();
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
    mShaderBindingTableHelper.Reset();
    D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle = mRayTraceHeap->GetHeapGPUStart();
    uint64 *heapPointer = reinterpret_cast<uint64*>(srvUavHeapHandle.ptr); //necessary reinterpret according to docs

    mShaderBindingTableHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });
    mShaderBindingTableHelper.AddMissProgram(L"Miss", {});
    mShaderBindingTableHelper.AddHitGroup(L"HitGroup", {});

    uint32_t sbtSize = mShaderBindingTableHelper.ComputeSBTSize(mDirect3DManager->GetRayTraceDevice());
    mShaderBindingTableStorage = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(sbtSize, D3D12_RESOURCE_STATE_GENERIC_READ, RayTraceBuffer::RayTraceBufferType_Shader_Binding_Table_Storage);

    mShaderBindingTableHelper.Generate(mShaderBindingTableStorage->GetResource(), mRayTraceStateObjectProperties);
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
    uavSrvHandle.ptr += mRayTraceHeap->GetDescriptorSize() * 2; //TDA: clean this up
    mDirect3DManager->GetDevice()->CopyDescriptorsSimple(1, uavSrvHandle, mCameraBuffers[mDirect3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    RayTraceContext *rayTraceContext = mDirect3DManager->GetContextManager()->GetRayTraceContext();
    rayTraceContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mRayTraceHeap->GetHeap());

    rayTraceContext->TransitionResource(mRayTraceRenderTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

    uint32 rayGenerationSectionSizeInBytes = mShaderBindingTableHelper.GetRayGenSectionSize();
    uint32 missSectionSizeInBytes = mShaderBindingTableHelper.GetMissSectionSize();
    uint32 hitGroupsSectionSize = mShaderBindingTableHelper.GetHitGroupSectionSize();

    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
    dispatchDesc.RayGenerationShaderRecord.StartAddress = mShaderBindingTableStorage->GetGpuAddress();
    dispatchDesc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;
    dispatchDesc.MissShaderTable.StartAddress = mShaderBindingTableStorage->GetGpuAddress() + rayGenerationSectionSizeInBytes;
    dispatchDesc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
    dispatchDesc.MissShaderTable.StrideInBytes = mShaderBindingTableHelper.GetMissEntrySize();
    dispatchDesc.HitGroupTable.StartAddress = dispatchDesc.MissShaderTable.StartAddress + missSectionSizeInBytes;
    dispatchDesc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
    dispatchDesc.HitGroupTable.StrideInBytes = mShaderBindingTableHelper.GetHitGroupEntrySize();
    dispatchDesc.Width = mRayTraceRenderTarget->GetWidth();
    dispatchDesc.Height = mRayTraceRenderTarget->GetHeight();

    rayTraceContext->DispatchRays(mRayTraceStateObject, dispatchDesc);
    rayTraceContext->Flush(mDirect3DManager->GetContextManager()->GetQueueManager(), true);
}