#include "Render/RayTrace/RayTraceAccelerationStructure.h"
#include "Render/Mesh/Mesh.h"

RayTraceAccelerationStructure::RayTraceAccelerationStructure(Direct3DManager *direct3DManager, RayTraceAccelerationStructureFlags flags)
{
    mDirect3DManager = direct3DManager;
    mFlags = flags;
    mTopLevelResultBuffer = NULL;
    mTopLevelScratchBuffer = NULL;
    mInstanceBuffer = NULL;
    mBottomLevelResultBuffer = NULL;
    mBottomLevelScratchBuffer = NULL;

    //can't have both
    Application::Assert(((mFlags & RayTraceAccelerationStructureFlags::FAST_BUILD) && (mFlags & RayTraceAccelerationStructureFlags::FAST_TRACE)) == 0);
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

void RayTraceAccelerationStructure::AddMesh(VertexBuffer *vertexBuffer, IndexBuffer *indexBuffer, RayTraceBuffer *transformBuffer, uint32 numVertices, uint32 numIndices)
{
    /*
    If Transform is NULL the vertices will not be transformed. Using Transform may result in increased computation and/or memory requirements
    for the acceleration structure build. The memory pointed to must be in state D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE.  
    The address must be aligned to 16 bytes (D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT).
    */

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetGpuAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(MeshVertex);
    geometryDesc.Triangles.VertexCount = numVertices;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.Transform3x4 = transformBuffer->GetGpuAddress();
    geometryDesc.Triangles.IndexBuffer = indexBuffer ? indexBuffer->GetGpuAddress() : 0;
    geometryDesc.Triangles.IndexFormat = indexBuffer ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
    geometryDesc.Triangles.IndexCount = indexBuffer ? numIndices : 0;
    geometryDesc.Flags = (mFlags & RayTraceAccelerationStructureFlags::IS_OPAQUE) ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

    mRayTraceGeometryDescs.Add(geometryDesc);
}

void RayTraceAccelerationStructure::BuildBottomLevelStructure(bool isUpdate)
{
    if (isUpdate)
    {
        Application::Assert(mFlags & RayTraceAccelerationStructureFlags::CAN_UPDATE);
    }

    const D3D12_RAYTRACING_GEOMETRY_DESC *geometryDescs = mRayTraceGeometryDescs.GetInnerArray();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    buildFlags |= (mFlags & RayTraceAccelerationStructureFlags::CAN_UPDATE) ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    buildFlags |= (mFlags & RayTraceAccelerationStructureFlags::FAST_BUILD) ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    buildFlags |= (mFlags & RayTraceAccelerationStructureFlags::FAST_TRACE) ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    //TDA: review the flags here to see what should really be used
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS structurePrebuildInfoDesc = {};
    structurePrebuildInfoDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    structurePrebuildInfoDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    structurePrebuildInfoDesc.NumDescs = mRayTraceGeometryDescs.CurrentSize();
    structurePrebuildInfoDesc.pGeometryDescs = geometryDescs;
    structurePrebuildInfoDesc.Flags = buildFlags;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    mDirect3DManager->GetRayTraceDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&structurePrebuildInfoDesc, &prebuildInfo);

    mBottomLevelScratchBufferSize = MathHelper::AlignU64(prebuildInfo.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    mBottomLevelResultBufferSize = MathHelper::AlignU64(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    mBottomLevelScratchBuffer = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(mBottomLevelScratchBufferSize, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, RayTraceBuffer::RayTraceBufferType_Acceleration_Structure);
    mBottomLevelResultBuffer = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(mBottomLevelResultBufferSize, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, RayTraceBuffer::RayTraceBufferType_Acceleration_Structure);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelAccelerationDesc = {};
    bottomLevelAccelerationDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelAccelerationDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelAccelerationDesc.Inputs.NumDescs = mRayTraceGeometryDescs.CurrentSize();
    bottomLevelAccelerationDesc.Inputs.pGeometryDescs = geometryDescs;
    bottomLevelAccelerationDesc.Inputs.Flags = isUpdate ? (buildFlags | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) : buildFlags;
    bottomLevelAccelerationDesc.ScratchAccelerationStructureData = mBottomLevelScratchBuffer->GetGpuAddress();
    bottomLevelAccelerationDesc.DestAccelerationStructureData = mBottomLevelResultBuffer->GetGpuAddress();
    bottomLevelAccelerationDesc.SourceAccelerationStructureData = isUpdate ? mBottomLevelResultBuffer->GetGpuAddress() : 0;

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
    if (isUpdate)
    {
        Application::Assert(mFlags & RayTraceAccelerationStructureFlags::CAN_UPDATE);
    }

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    buildFlags |= (mFlags & RayTraceAccelerationStructureFlags::CAN_UPDATE) ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    buildFlags |= (mFlags & RayTraceAccelerationStructureFlags::FAST_BUILD) ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    buildFlags |= (mFlags & RayTraceAccelerationStructureFlags::FAST_TRACE) ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS structurePrebuildInfoDesc = {};
    structurePrebuildInfoDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    structurePrebuildInfoDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    structurePrebuildInfoDesc.NumDescs = mInstanceDescs.CurrentSize();
    structurePrebuildInfoDesc.pGeometryDescs = NULL;
    structurePrebuildInfoDesc.Flags = buildFlags;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    mDirect3DManager->GetRayTraceDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&structurePrebuildInfoDesc, &prebuildInfo);

    mTopLevelScratchBufferSize = MathHelper::AlignU64(prebuildInfo.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    mTopLevelResultBufferSize = MathHelper::AlignU64(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    mTopLevelInstanceBufferSize = MathHelper::AlignU64(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * mInstanceDescs.CurrentSize(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    mTopLevelScratchBuffer = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(mTopLevelScratchBufferSize, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, RayTraceBuffer::RayTraceBufferType_Acceleration_Structure);
    mTopLevelResultBuffer = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(mTopLevelResultBufferSize, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, RayTraceBuffer::RayTraceBufferType_Acceleration_Structure, true);
    mInstanceBuffer = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(mTopLevelResultBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, RayTraceBuffer::RayTraceBufferType_Instancing);

    Application::Assert(mInstanceDescs.CurrentSize() > 0);
    const void *instanceDescs = mInstanceDescs.GetInnerArray();
    mInstanceBuffer->MapInstanceDescData(instanceDescs, mInstanceDescs.CurrentSize());

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelAccelerationDesc = {};
    topLevelAccelerationDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelAccelerationDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelAccelerationDesc.Inputs.NumDescs = mInstanceDescs.CurrentSize();
    topLevelAccelerationDesc.Inputs.InstanceDescs = mInstanceBuffer->GetGpuAddress();
    topLevelAccelerationDesc.Inputs.Flags = isUpdate ? (buildFlags | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) : buildFlags;
    topLevelAccelerationDesc.ScratchAccelerationStructureData = mTopLevelScratchBuffer->GetGpuAddress();
    topLevelAccelerationDesc.DestAccelerationStructureData = mTopLevelResultBuffer->GetGpuAddress();
    topLevelAccelerationDesc.SourceAccelerationStructureData = isUpdate ? mTopLevelResultBuffer->GetGpuAddress() : 0;

    RayTraceContext *rayTraceContext = mDirect3DManager->GetContextManager()->GetRayTraceContext();
    rayTraceContext->BuildAccelerationStructure(topLevelAccelerationDesc);
    rayTraceContext->InsertUAVBarrier(mTopLevelResultBuffer, true);
}