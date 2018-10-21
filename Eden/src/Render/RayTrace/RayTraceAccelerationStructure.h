#pragma once
#include "Render/DirectX/Direct3DManager.h"

/*
Information to refer back to.
Ray trace structure updates are constrained by some strong rules:

Bottom level
------------
For triangle-based data, only vertex positions and transforms can be updated (but cannot switch between NULL and non-NULL transforms).
-Number of geometries cannot change.
-Vertex count cannot change.
-Index buffer contents cannot change.
-If a bottom-level acceleration structure changes and is pointed to by top-level structure, the top-level acceleration structure is 
stale and must either be rebuilt or updated before they are valid to use again.

Top level
-----------
-Instance descs can change to completely different bottom level structures, but number of instance descs cannot change.
-"Only updating instead of rebuilding is rarely the right thing to do.  Rebuilds for a few thousand instances are very fast, and having a 
good quality top-level acceleration structure can have a significant payoff (bad quality has a higher cost further up in the tree)."

Important information regarding opaque geometry:
When rays encounter this geometry, the geometry acts as if no any hit shader is present. Opaque geo enables heavy ray optimizations.  
Note that this behavior can be overridden on a per-instance basis with D3D12_RAYTRACING_INSTANCE_FLAGS and on a per-ray basis using Ray flags in TraceRay().


Use UAV barriers to synchronize between structure writes (when building/rebuilding acceleration structures) and reads (from DispatchRays)
*/

class Mesh;

enum RayTraceAccelerationStructureFlags
{
    FAST_BUILD = 1 << 0,
    FAST_TRACE = 1 << 1,
    CAN_UPDATE = 1 << 2,
    IS_OPAQUE  = 1 << 3,
};

enum RayTraceAccelerationStructureType
{
    //For fully dynamic geometry, rebuilt per frame, like particles. No updates, just full builds.
    RayTraceAccelerationStructureType_Fastest_Build = 0, 

    //For low-quality LOD dynamic objects that wouldn't be hit by many rays.
    RayTraceAccelerationStructureType_Fast_Build_With_Update,

    //For static geometry. No updates, just full builds.
    RayTraceAccelerationStructureType_Fastest_Trace,

    //For high quality dynamic objects that will be hit by many rays.
    RayTraceAccelerationStructureType_Fast_Trace_With_Update,

    RayTraceAccelerationStructureType_Num_Types
};

class RayTraceAccelerationStructure
{
public:
    RayTraceAccelerationStructure(Direct3DManager *direct3DManager, RayTraceAccelerationStructureFlags flags);
    ~RayTraceAccelerationStructure();

    void ClearDescs();
    void AddMesh(VertexBuffer *vertexBuffer, IndexBuffer *indexBuffer, RayTraceBuffer *transformBuffer, uint32 numVertices, uint32 numIndices);
    void BuildBottomLevelStructure(bool isUpdate);
    void AddBottomLevelInstance(D3DXMATRIX transform, uint32 instanceID, uint32 hitGroupIndex);
    void BuildTopLevelStructure(bool isUpdate);

    RayTraceBuffer *GetTopLevelResultBuffer() { return mTopLevelResultBuffer; }

private:
    struct BottomLevelInstance
    {
        ID3D12Resource *BottomLevelResource;
        D3DXMATRIX Transform;
        uint32 InstanceID;
        uint32 HitGroupIndex;
    };

    Direct3DManager *mDirect3DManager;

    RayTraceAccelerationStructureFlags mFlags;
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