#pragma once
#include "Render/DirectX/Direct3DManager.h"

class Mesh;

class RayTraceAccelerationStructure
{
public:
    RayTraceAccelerationStructure(Direct3DManager *direct3DManager, bool canUpdate);
    ~RayTraceAccelerationStructure();

    void ClearDescs();
    void AddMesh(VertexBuffer *vertexBuffer, IndexBuffer *indexBuffer, uint32 numVertices, uint32 numIndices);
    void BuildBottomLevelStructure(bool isUpdate);
    void AddBottomLevelInstance(D3DXMATRIX transform, uint32 instanceID, uint32 hitGroupIndex);
    void BuildTopLevelStructure(bool isUpdate);

    struct RTXVertex
    {
        Vector3 Position;
    };

private:
    struct BottomLevelInstance
    {
        ID3D12Resource *BottomLevelResource;
        D3DXMATRIX Transform;
        uint32 InstanceID;
        uint32 HitGroupIndex;
    };

    Direct3DManager *mDirect3DManager;

    bool mCanUpdate;
    DynamicArray<D3D12_RAYTRACING_GEOMETRY_DESC> mRayTraceGeometryDescs;
    DynamicArray<D3D12_RAYTRACING_INSTANCE_DESC> mInstanceDescs;

    uint64 mBottomLevelScratchBufferSize;
    uint64 mBottomLevelResultBufferSize;
    RayTraceBuffer *mBottomLevelScratchBuffer;
    RayTraceBuffer *mBottomLevelResultBuffer;

    uint64 mTopLevelScratchBufferSize;
    uint64 mTopLevelResultBufferSize;
    uint64 mTopLevelInstanceBufferSize;
    RayTraceBuffer *mTopLevelScratchBuffer;
    RayTraceBuffer *mTopLevelResultBuffer;
    RayTraceBuffer *mInstanceBuffer;
};