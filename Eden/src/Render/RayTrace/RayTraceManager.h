#pragma once
#include "Render/RayTrace/RayTraceAccelerationStructure.h"
#include "Render/RayTrace/RayTraceShaderManager.h"

class Camera;

class RayTraceManager
{
public:
    RayTraceManager(Direct3DManager *direct3DManager, RootSignatureManager *rootSignatureManager);
    ~RayTraceManager();

    void AddMeshToAccelerationStructure(Mesh *mesh, const D3DXMATRIX &transform, RayTraceAccelerationStructureType structureType);
    RenderTarget *GetRenderTarget() { return mRayTraceRenderTarget; }

    void Update(Camera *camera, Vector3 lightDirection);
    void RenderBarycentricRayTrace();
    void RenderShadowRayTrace(DepthStencilTarget *depthStencilTarget);

private:
    enum RayTracingState
    {
        RayTracingState_Uninitialized = 0,
        RayTracingState_Acceleration_Structure_Creation,
        RayTracingState_Ready_For_Dispatch
    };

    struct RayMesh
    {
        Mesh *Mesh;
        RayTraceBuffer *TransformBuffer;
    };

    struct RayTraceStructureGroup
    {
        RayTraceStructureGroup()
        {
            AccelerationStructure = NULL;
            NeedsUpdate = false;
            NeedsRebuild = false;
        }

        RayTraceAccelerationStructure *AccelerationStructure;
        DynamicArray<RayMesh> Meshes;
        bool NeedsUpdate;
        bool NeedsRebuild;
    };

    void UpdateCameraBuffers(Camera *camera, Vector3 lightDirection);
    void LoadRayTracePipelines();
    void CreateAccelerationStructure(RayTraceAccelerationStructureType structureType);

    Direct3DManager *mDirect3DManager;

    RayTraceShaderManager *mRayShaderManager;
    RayTraceStructureGroup* mRayTraceAccelerationStructures[RayTraceAccelerationStructureType_Num_Types];
    
    RenderTarget *mRayTraceRenderTarget;
    DescriptorHeap *mRayTraceHeap;
    ConstantBuffer *mCameraBuffers[FRAME_BUFFER_COUNT];
    ConstantBuffer *mCameraShadowBuffers[FRAME_BUFFER_COUNT];

    uint64 mRayTraceFence;
    bool mReadyToRender;
    RayTraceShaderManager::RayTracePSO *mBarycentricRayTracePSO;
    RayTraceShaderManager::RayTracePSO *mShadowRayTracePSO;
    DynamicArray<RayTraceBuffer*> mTransformBufferCache;
};