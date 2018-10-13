#include "Render/RayTrace/RayTraceManager.h"
#include "Render/Mesh/Mesh.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"
#include "Camera/Camera.h"

//TDA: Implement update support
RayTraceManager::RayTraceManager(Direct3DManager *direct3DManager, RootSignatureManager *rootSignatureManager)
{
    mDirect3DManager = direct3DManager;

    mAccelerationStructure = new RayTraceAccelerationStructure(mDirect3DManager, false);
    mRayShaderManager = new RayTraceShaderManager(direct3DManager, rootSignatureManager);

    mShouldBuildAccelerationStructure = false;
    mAccelerationStructureFence = 0;
    mRayTracingState = RayTracingState_Uninitialized;
    mRayTraceHeap = NULL;

    Vector2 screenSize = mDirect3DManager->GetScreenSize();
    mRayTraceRenderTarget = mDirect3DManager->GetContextManager()->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R8G8B8A8_UNORM, true, 1, 1, 0);

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mCameraBuffers[i] = mDirect3DManager->GetContextManager()->CreateConstantBuffer(sizeof(CameraRayTraceBuffer));
    }
    
    LoadRayTracePipelines();
}

RayTraceManager::~RayTraceManager()
{
    mBarycentricRayTracePSO = NULL;

    mDirect3DManager->GetContextManager()->FreeRenderTarget(mRayTraceRenderTarget);
    mRayTraceRenderTarget = NULL;

    if (mRayTraceHeap)
    {
        delete mRayTraceHeap;
    }

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mDirect3DManager->GetContextManager()->FreeConstantBuffer(mCameraBuffers[i]);
        mCameraBuffers[i] = NULL;
    }

    if (mAccelerationStructure)
    {
        delete mAccelerationStructure;
    }

    delete mRayShaderManager;
}

void RayTraceManager::QueueRayTraceAccelerationStructureCreation(Mesh *mesh)
{
    mShouldBuildAccelerationStructure = true;
    mMesh = mesh;
}

void RayTraceManager::UpdateCameraBuffer(Camera *camera)
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
}

void RayTraceManager::Update(Camera *camera)
{
    switch (mRayTracingState)
    {
    case RayTracingState_Uninitialized:
        if (mShouldBuildAccelerationStructure)
        {
            bool readyForCreation = true;
            readyForCreation &= mMesh->IsReady();

            if (readyForCreation)
            {
                mAccelerationStructureFence = CreateAccelerationStructures();
                mRayTracingState = RayTracingState_Acceleration_Structure_Creation;
            }
        }
        break;
    case RayTracingState_Acceleration_Structure_Creation:
        {
            uint64 currentFence = mDirect3DManager->GetContextManager()->GetQueueManager()->GetGraphicsQueue()->PollCurrentFenceValue();
            if (mAccelerationStructureFence <= currentFence)
            {
                BuildHeap();
                mRayTracingState = RayTracingState_Ready_For_Dispatch;
            }
        }
        break;
    case RayTracingState_Ready_For_Dispatch:
        {
            UpdateCameraBuffer(camera);
            DispatchRayTrace();
        }
        break;
    default:
        Application::Assert(false);
        break;
    }
}

void RayTraceManager::LoadRayTracePipelines()
{
    //TDA: clean up memory use of everything in this file
    mBarycentricRayTracePSO = mRayShaderManager->GetRayTracePipeline("Barycentric");
}

uint64 RayTraceManager::CreateAccelerationStructures()
{
    D3DXMATRIX structureMatrix;
    D3DXMatrixIdentity(&structureMatrix);

    mAccelerationStructure->ClearDescs();
    mAccelerationStructure->AddMesh(mMesh->GetVertexBuffer(), NULL, mMesh->GetVertexCount(), 0);
    mAccelerationStructure->BuildBottomLevelStructure(false);
    mAccelerationStructure->AddBottomLevelInstance(structureMatrix, 0, 0);
    mAccelerationStructure->BuildTopLevelStructure(false);

    return mDirect3DManager->GetContextManager()->GetRayTraceContext()->Flush(mDirect3DManager->GetContextManager()->GetQueueManager());
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
    rayTraceContext->TransitionResource(mRayTraceRenderTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
    rayTraceContext->SetComputeRootSignature(mBarycentricRayTracePSO->GlobalRootSignature);
    rayTraceContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mRayTraceHeap->GetHeap());
    rayTraceContext->SetComputeDescriptorTable(0, mRayTraceHeap->GetHeapGPUStart());
    rayTraceContext->SetRayPipelineState(mBarycentricRayTracePSO->RayTraceStateObject);
    
    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
    dispatchDesc.RayGenerationShaderRecord.StartAddress = mBarycentricRayTracePSO->GenShaderTable->GetGpuAddress();
    dispatchDesc.RayGenerationShaderRecord.SizeInBytes = mBarycentricRayTracePSO->GenShaderTable->GetResource()->GetDesc().Width;
    dispatchDesc.MissShaderTable.StartAddress = mBarycentricRayTracePSO->MissShaderTable->GetGpuAddress();
    dispatchDesc.MissShaderTable.SizeInBytes = mBarycentricRayTracePSO->MissShaderTable->GetResource()->GetDesc().Width;
    dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes;
    dispatchDesc.HitGroupTable.StartAddress = mBarycentricRayTracePSO->HitGroupShaderTable->GetGpuAddress();
    dispatchDesc.HitGroupTable.SizeInBytes = mBarycentricRayTracePSO->HitGroupShaderTable->GetResource()->GetDesc().Width;
    dispatchDesc.HitGroupTable.StrideInBytes = dispatchDesc.HitGroupTable.SizeInBytes;
    dispatchDesc.Width = mRayTraceRenderTarget->GetWidth();
    dispatchDesc.Height = mRayTraceRenderTarget->GetHeight();
    dispatchDesc.Depth = 1;

    rayTraceContext->DispatchRays(dispatchDesc);
    rayTraceContext->Flush(mDirect3DManager->GetContextManager()->GetQueueManager()); //TDA fence this later?
}