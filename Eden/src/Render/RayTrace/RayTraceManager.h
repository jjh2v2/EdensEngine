#pragma once
#include "Render/RayTrace/RayTraceAccelerationStructure.h"
#include "Render/RayTrace/RayTraceShaderManager.h"

class Camera;

class RayTraceManager
{
public:
    RayTraceManager(Direct3DManager *direct3DManager, RootSignatureManager *rootSignatureManager);
    ~RayTraceManager();

    void QueueRayTraceAccelerationStructureCreation(Mesh *mesh);
    void Update(Camera *camera);
    bool GetIsReady() { return mRayTracingState == RayTracingState_Ready_For_Dispatch; }
    RenderTarget *GetRenderTarget() { return mRayTraceRenderTarget; }

private:
    enum RayTracingState
    {
        RayTracingState_Uninitialized = 0,
        RayTracingState_Acceleration_Structure_Creation,
        RayTracingState_Ready_For_Dispatch
    };

    void UpdateCameraBuffer(Camera *camera);
    void LoadRayTracePipelines();
    void BuildHeap();
    void DispatchRayTrace();
    uint64 CreateAccelerationStructures();

    Direct3DManager *mDirect3DManager;
    Mesh *mMesh;

    RayTraceShaderManager *mRayShaderManager;
    RayTraceAccelerationStructure *mAccelerationStructure;
    
    RenderTarget *mRayTraceRenderTarget;
    DescriptorHeap *mRayTraceHeap;
    ConstantBuffer *mCameraBuffers[FRAME_BUFFER_COUNT];

    RayTraceShaderManager::RayTracePSO *mBarycentricRayTracePSO;

    bool mShouldBuildAccelerationStructure;
    uint64 mAccelerationStructureFence;
    RayTracingState mRayTracingState;
};