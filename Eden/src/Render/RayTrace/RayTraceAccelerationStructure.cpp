#include "Render/RayTrace/RayTraceAccelerationStructure.h"
#include "Render/Mesh/Mesh.h"

RayTraceAccelerationStructure::RayTraceAccelerationStructure(Direct3DManager *direct3DManager, bool canUpdate)
{
    mDirect3DManager = direct3DManager;
    mCanUpdate = canUpdate;
    mTopLevelResultBuffer = NULL;
    mTopLevelScratchBuffer = NULL;
    mInstanceBuffer = NULL;
    mBottomLevelResultBuffer = NULL;
    mBottomLevelScratchBuffer = NULL;
}

RayTraceAccelerationStructure::~RayTraceAccelerationStructure()
{
    Direct3DContextManager *contextManager = mDirect3DManager->GetContextManager();

    if (mTopLevelResultBuffer)
    {
        contextManager->FreeRayTraceBuffer(mTopLevelResultBuffer);
    }
    
    if (mTopLevelScratchBuffer)
    {
        contextManager->FreeRayTraceBuffer(mTopLevelScratchBuffer);
    }
    
    if (mInstanceBuffer)
    {
        contextManager->FreeRayTraceBuffer(mInstanceBuffer);
    }

    if (mBottomLevelResultBuffer)
    {
        contextManager->FreeRayTraceBuffer(mBottomLevelResultBuffer);
    }

    if (mBottomLevelScratchBuffer)
    {
        contextManager->FreeRayTraceBuffer(mBottomLevelScratchBuffer);
    }
}

void RayTraceAccelerationStructure::ClearDescs()
{
    mRayTraceGeometryDescs.ClearFast();
    mInstanceDescs.ClearFast();
}

void RayTraceAccelerationStructure::AddMesh(VertexBuffer *vertexBuffer, IndexBuffer *indexBuffer, uint32 numVertices, uint32 numIndices)
{
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetGpuAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(RTXVertex);
    geometryDesc.Triangles.VertexCount = numVertices;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.Transform = NULL;
    geometryDesc.Triangles.IndexBuffer = indexBuffer ? indexBuffer->GetGpuAddress() : 0;
    geometryDesc.Triangles.IndexFormat = indexBuffer ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
    geometryDesc.Triangles.IndexCount = indexBuffer ? numIndices : 0;
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    mRayTraceGeometryDescs.Add(geometryDesc);
}

void RayTraceAccelerationStructure::BuildBottomLevelStructure(bool isUpdate)
{
    const D3D12_RAYTRACING_GEOMETRY_DESC *geometryDescs = mRayTraceGeometryDescs.GetInnerArray();

    //TDA: review the flags here to see what should really be used
    D3D12_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_DESC structurePrebuildInfoDesc = {};
    structurePrebuildInfoDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    structurePrebuildInfoDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    structurePrebuildInfoDesc.NumDescs = mRayTraceGeometryDescs.CurrentSize();
    structurePrebuildInfoDesc.pGeometryDescs = geometryDescs;
    structurePrebuildInfoDesc.Flags = mCanUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    mDirect3DManager->GetRayTraceDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&structurePrebuildInfoDesc, &prebuildInfo);

    mBottomLevelScratchBufferSize = MathHelper::AlignU64(prebuildInfo.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    mBottomLevelResultBufferSize = MathHelper::AlignU64(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    mBottomLevelScratchBuffer = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(mBottomLevelScratchBufferSize, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, RayTraceBuffer::RayTraceBufferType_Acceleration_Structure);
    mBottomLevelResultBuffer = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(mBottomLevelResultBufferSize, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, RayTraceBuffer::RayTraceBufferType_Acceleration_Structure);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelAccelerationDesc = {};
    bottomLevelAccelerationDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelAccelerationDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelAccelerationDesc.NumDescs = mRayTraceGeometryDescs.CurrentSize();
    bottomLevelAccelerationDesc.pGeometryDescs = geometryDescs;
    bottomLevelAccelerationDesc.Flags = mCanUpdate && isUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    bottomLevelAccelerationDesc.ScratchAccelerationStructureData.SizeInBytes = mBottomLevelScratchBufferSize;
    bottomLevelAccelerationDesc.ScratchAccelerationStructureData.StartAddress = mBottomLevelScratchBuffer->GetGpuAddress();
    bottomLevelAccelerationDesc.DestAccelerationStructureData.SizeInBytes = mBottomLevelResultBufferSize;
    bottomLevelAccelerationDesc.DestAccelerationStructureData.StartAddress = mBottomLevelResultBuffer->GetGpuAddress();
    bottomLevelAccelerationDesc.SourceAccelerationStructureData = mCanUpdate && isUpdate ? mBottomLevelResultBuffer->GetGpuAddress() : 0;

    RayTraceContext *rayTraceContext = mDirect3DManager->GetContextManager()->GetRayTraceContext();
    rayTraceContext->BuildAccelerationStructure(bottomLevelAccelerationDesc);
    rayTraceContext->InsertUAVBarrier(mBottomLevelResultBuffer, true);
}

//TDA: allow for multiple bottom level structures
void RayTraceAccelerationStructure::AddBottomLevelInstance(D3DXMATRIX transform, uint32 instanceID, uint32 hitGroupIndex)
{
    D3DXMatrixTranspose(&transform, &transform);

    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.AccelerationStructure = mBottomLevelResultBuffer->GetGpuAddress();
    instanceDesc.InstanceID = instanceID;
    instanceDesc.InstanceContributionToHitGroupIndex = hitGroupIndex;
    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDesc.InstanceMask = 0xFF;
    memcpy(&instanceDesc.Transform, transform, sizeof(instanceDesc.Transform));

    mInstanceDescs.Add(instanceDesc);
}

void RayTraceAccelerationStructure::BuildTopLevelStructure(bool isUpdate)
{
    D3D12_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_DESC structurePrebuildInfoDesc = {};
    structurePrebuildInfoDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    structurePrebuildInfoDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    structurePrebuildInfoDesc.NumDescs = mInstanceDescs.CurrentSize();
    structurePrebuildInfoDesc.pGeometryDescs = NULL;
    structurePrebuildInfoDesc.Flags = mCanUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    mDirect3DManager->GetRayTraceDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&structurePrebuildInfoDesc, &prebuildInfo);

    mTopLevelScratchBufferSize = MathHelper::AlignU64(prebuildInfo.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    mTopLevelResultBufferSize = MathHelper::AlignU64(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    mTopLevelInstanceBufferSize = MathHelper::AlignU64(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * mInstanceDescs.CurrentSize(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    mTopLevelScratchBuffer = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(mTopLevelScratchBufferSize, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, RayTraceBuffer::RayTraceBufferType_Acceleration_Structure);
    mTopLevelResultBuffer = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(mTopLevelResultBufferSize, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, RayTraceBuffer::RayTraceBufferType_Acceleration_Structure);
    mInstanceBuffer = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(mTopLevelResultBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, RayTraceBuffer::RayTraceBufferType_Instancing);

    Application::Assert(mInstanceDescs.CurrentSize() > 0);
    const void *instanceDescs = mInstanceDescs.GetInnerArray();
    mInstanceBuffer->MapInstanceDescData(instanceDescs, mInstanceDescs.CurrentSize());

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelAccelerationDesc = {};
    topLevelAccelerationDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelAccelerationDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelAccelerationDesc.NumDescs = mInstanceDescs.CurrentSize();
    topLevelAccelerationDesc.InstanceDescs = mInstanceBuffer->GetGpuAddress();
    topLevelAccelerationDesc.ScratchAccelerationStructureData.SizeInBytes = mTopLevelScratchBufferSize;
    topLevelAccelerationDesc.ScratchAccelerationStructureData.StartAddress = mTopLevelScratchBuffer->GetGpuAddress();
    topLevelAccelerationDesc.DestAccelerationStructureData.SizeInBytes = mTopLevelResultBufferSize;
    topLevelAccelerationDesc.DestAccelerationStructureData.StartAddress = mTopLevelResultBuffer->GetGpuAddress();
    topLevelAccelerationDesc.Flags = mCanUpdate && isUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    topLevelAccelerationDesc.SourceAccelerationStructureData = mCanUpdate && isUpdate ? mTopLevelResultBuffer->GetGpuAddress() : 0;

    RayTraceContext *rayTraceContext = mDirect3DManager->GetContextManager()->GetRayTraceContext();
    rayTraceContext->BuildAccelerationStructure(topLevelAccelerationDesc);
    rayTraceContext->InsertUAVBarrier(mTopLevelResultBuffer, true);
}