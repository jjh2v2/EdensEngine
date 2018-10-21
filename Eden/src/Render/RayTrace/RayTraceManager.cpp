#include "Render/RayTrace/RayTraceManager.h"
#include "Render/Mesh/Mesh.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"
#include "Camera/Camera.h"

RayTraceManager::RayTraceManager(Direct3DManager *direct3DManager, RootSignatureManager *rootSignatureManager)
{
    mDirect3DManager = direct3DManager;
    mRayTraceHeap = NULL;

    for (uint32 i = 0; i < RayTraceAccelerationStructureType_Num_Types; i++)
    {
        mRayTraceAccelerationStructures[i] = new RayTraceStructureGroup();
    }

    mRayShaderManager = new RayTraceShaderManager(direct3DManager, rootSignatureManager);

    Vector2 screenSize = mDirect3DManager->GetScreenSize();
    mRayTraceRenderTarget = mDirect3DManager->GetContextManager()->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R8G8B8A8_UNORM, true, 1, 1, 0);
    mRayTraceHeap = new DescriptorHeap(mDirect3DManager->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RAY_TRACE_DESCRIPTOR_HEAP_SIZE, true);

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mCameraBuffers[i] = mDirect3DManager->GetContextManager()->CreateConstantBuffer(sizeof(CameraRayTraceBuffer));
    }
    
    mRayTraceFence = 0;

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

    for (uint32 i = 0; i < RayTraceAccelerationStructureType_Num_Types; i++)
    {
        if (mRayTraceAccelerationStructures[i])
        {
            delete mRayTraceAccelerationStructures[i]->AccelerationStructure;
            delete mRayTraceAccelerationStructures[i];
        }
    }

    for (uint32 i = 0; i < mTransformBufferCache.CurrentSize(); i++)
    {
        mDirect3DManager->GetContextManager()->FreeRayTraceBuffer(mTransformBufferCache[i]);
    }

    delete mRayShaderManager;
}

void RayTraceManager::AddMeshToAccelerationStructure(Mesh *mesh, const D3DXMATRIX &transform, RayTraceAccelerationStructureType structureType)
{
    RayMesh newRayMesh;
    newRayMesh.Mesh = mesh;
    newRayMesh.TransformBuffer = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(RAY_TRACE_TRANFORM_BUFFER_SIZE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, RayTraceBuffer::RayTraceBufferType_Transform);
    newRayMesh.TransformBuffer->MapTransform(transform, RAY_TRACE_TRANFORM_BUFFER_SIZE);

    mRayTraceAccelerationStructures[structureType]->Meshes.Add(newRayMesh);
    mRayTraceAccelerationStructures[structureType]->NeedsRebuild = true;

    mTransformBufferCache.Add(newRayMesh.TransformBuffer);
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

void RayTraceManager::RenderRayTrace(Camera *camera)
{
    for (uint32 i = 0; i < RayTraceAccelerationStructureType_Num_Types; i++)
    {
        if (mRayTraceAccelerationStructures[i]->NeedsRebuild)
        {
            if (!mRayTraceAccelerationStructures[i]->AccelerationStructure)
            {
                switch (i)
                {
                case RayTraceAccelerationStructureType_Fastest_Build:
                    mRayTraceAccelerationStructures[i]->AccelerationStructure = new RayTraceAccelerationStructure(mDirect3DManager, RayTraceAccelerationStructureFlags(RayTraceAccelerationStructureFlags::FAST_BUILD | RayTraceAccelerationStructureFlags::IS_OPAQUE));
                    break;
                case RayTraceAccelerationStructureType_Fast_Build_With_Update:
                    mRayTraceAccelerationStructures[i]->AccelerationStructure = new RayTraceAccelerationStructure(mDirect3DManager, RayTraceAccelerationStructureFlags(RayTraceAccelerationStructureFlags::FAST_BUILD | RayTraceAccelerationStructureFlags::IS_OPAQUE | RayTraceAccelerationStructureFlags::CAN_UPDATE));
                    break;
                case RayTraceAccelerationStructureType_Fastest_Trace:
                    mRayTraceAccelerationStructures[i]->AccelerationStructure = new RayTraceAccelerationStructure(mDirect3DManager, RayTraceAccelerationStructureFlags(RayTraceAccelerationStructureFlags::FAST_TRACE | RayTraceAccelerationStructureFlags::IS_OPAQUE));
                    break;
                case RayTraceAccelerationStructureType_Fast_Trace_With_Update:
                    mRayTraceAccelerationStructures[i]->AccelerationStructure = new RayTraceAccelerationStructure(mDirect3DManager, RayTraceAccelerationStructureFlags(RayTraceAccelerationStructureFlags::FAST_TRACE | RayTraceAccelerationStructureFlags::IS_OPAQUE | RayTraceAccelerationStructureFlags::CAN_UPDATE));
                    break;
                default:
                    Application::Assert(false);
                    break;
                }
            }
            
            CreateAccelerationStructure((RayTraceAccelerationStructureType)i);
            mRayTraceAccelerationStructures[i]->NeedsRebuild = false;
        }
    }

    if(!mRayTraceAccelerationStructures[RayTraceAccelerationStructureType_Fastest_Trace]->NeedsRebuild && mRayTraceAccelerationStructures[RayTraceAccelerationStructureType_Fastest_Trace]->Meshes.CurrentSize() > 0)
    {
        UpdateCameraBuffer(camera);
        DispatchRayTrace();
    }
}

void RayTraceManager::LoadRayTracePipelines()
{
    mBarycentricRayTracePSO = mRayShaderManager->GetRayTracePipeline("Barycentric");
}

void RayTraceManager::CreateAccelerationStructure(RayTraceAccelerationStructureType structureType)
{
    D3DXMATRIX structureMatrix;
    D3DXMatrixIdentity(&structureMatrix);

    mRayTraceAccelerationStructures[structureType]->AccelerationStructure->ClearDescs();

    uint32 numMeshes = mRayTraceAccelerationStructures[structureType]->Meshes.CurrentSize();
    for (uint32 i = 0; i < numMeshes; i++)
    {
        RayMesh &meshToAdd = mRayTraceAccelerationStructures[structureType]->Meshes[i];
        mRayTraceAccelerationStructures[structureType]->AccelerationStructure->AddMesh(meshToAdd.Mesh->GetVertexBuffer(), NULL, meshToAdd.TransformBuffer, meshToAdd.Mesh->GetVertexCount(), 0);
    }

    mRayTraceAccelerationStructures[structureType]->AccelerationStructure->BuildBottomLevelStructure(false);
    mRayTraceAccelerationStructures[structureType]->AccelerationStructure->AddBottomLevelInstance(structureMatrix, 0, 0);
    mRayTraceAccelerationStructures[structureType]->AccelerationStructure->BuildTopLevelStructure(false);
}

void RayTraceManager::DispatchRayTrace()
{
    D3D12_CPU_DESCRIPTOR_HANDLE uavSrvHandle = mRayTraceHeap->GetHeapCPUStart();
    mDirect3DManager->GetDevice()->CopyDescriptorsSimple(1, uavSrvHandle, mRayTraceRenderTarget->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    uavSrvHandle.ptr += mRayTraceHeap->GetDescriptorSize();
    mDirect3DManager->GetDevice()->CopyDescriptorsSimple(1, uavSrvHandle, mRayTraceAccelerationStructures[RayTraceAccelerationStructureType_Fastest_Trace]->AccelerationStructure->GetTopLevelResultBuffer()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    uavSrvHandle.ptr += mRayTraceHeap->GetDescriptorSize(); //TDA: clean this up
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

    //we shouldn't actually need to sync on this because command list execution (when submitted not together in a single flush) are completed in sequence
    mRayTraceFence = rayTraceContext->Flush(mDirect3DManager->GetContextManager()->GetQueueManager());
}