#pragma once
#include "Render/RayTrace/RayTraceAccelerationStructure.h"

class RayTraceManager
{
public:
    RayTraceManager(Direct3DManager *direct3DManager);
    ~RayTraceManager();

    void AddMeshToAccelerationStructure(Mesh *meshToAdd);
    void QueueRayTraceAccelerationStructureCreation();
    void Update();
    bool GetIsStructureReady() { return mIsStructureReady; }

private:
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
};