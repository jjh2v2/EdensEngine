#include "Render/RayTrace/RayTraceManager.h"
#include "Render/Mesh/Mesh.h"

//TDA: Implement update support
RayTraceManager::RayTraceManager(Direct3DManager *direct3DManager)
{
    mDirect3DManager = direct3DManager;

    mAccelerationStructure = new RayTraceAccelerationStructure(mDirect3DManager, false);
    mShouldBuildAccelerationStructure = false;
    mIsStructureReady = false;
    mStructureCreationFence = 0;
}

RayTraceManager::~RayTraceManager()
{
    Application::Assert(mStructureCreationFence == 0); //if the structure is in mid-flight build, nuking it would blow up

    if (mAccelerationStructure)
    {
        delete mAccelerationStructure;
    }

    if (mVertexBuffer)
    {
        delete mVertexBuffer;
    }

    if (mIndexBuffer)
    {
        delete mIndexBuffer;
    }
}

void RayTraceManager::AddMeshToAccelerationStructure(Mesh *meshToAdd)
{
    mMeshesForRayAcceleration.Add(meshToAdd);
}

void RayTraceManager::QueueRayTraceAccelerationStructureCreation()
{
    mShouldBuildAccelerationStructure = true;

    //create simple triangle
    mVertices[0].Position = Vector3( 0.0f, -0.7f, 1.0f);
    mVertices[1].Position = Vector3(-0.7f,  0.7f, 1.0f);
    mVertices[2].Position = Vector3( 0.7f,  0.7f, 1.0f);

    mIndices[0] = 0;
    mIndices[1] = 1;
    mIndices[2] = 2;

    mVertexBuffer = mDirect3DManager->GetContextManager()->CreateVertexBuffer(mVertices, sizeof(RayTraceAccelerationStructure::RTXVertex), 3 * sizeof(RayTraceAccelerationStructure::RTXVertex));
    mIndexBuffer = mDirect3DManager->GetContextManager()->CreateIndexBuffer(mIndices, sizeof(uint32) * 3);
}

void RayTraceManager::Update()
{
    //TDA: turn this back on when meshes are supported
    if (/*mMeshesForRayAcceleration.CurrentSize() == 0 ||*/ mIsStructureReady)
    {
        //nothing to do if there's no meshes or we've already built it (no rebuild support here yet)
        return;
    }

    if (mStructureCreationFence > 0)
    {
        uint64 currentFence = mDirect3DManager->GetContextManager()->GetQueueManager()->GetGraphicsQueue()->PollCurrentFenceValue();
        if (currentFence >= mStructureCreationFence)
        {
            mStructureCreationFence = 0;
            mIsStructureReady = true;
            return;
        }
    }

    if (mShouldBuildAccelerationStructure)
    {
        bool readyForCreation = true;

        //TDA: make this actually support meshes
        const uint32 numMeshes = mMeshesForRayAcceleration.CurrentSize();
        for (uint32 i = 0; i < numMeshes; i++)
        {
            readyForCreation &= mMeshesForRayAcceleration[i]->IsReady();
        }

        readyForCreation &= mVertexBuffer->GetIsReady();
        readyForCreation &= mIndexBuffer->GetIsReady();

        if (readyForCreation)
        {
            mAccelerationStructure->ClearDescs();

            for (uint32 i = 0; i < numMeshes; i++)
            {
                mAccelerationStructure->AddMesh(mVertexBuffer, mIndexBuffer, 3, 3);
            }

            D3DXMATRIX structureMatrix;
            D3DXMatrixIdentity(&structureMatrix);

            mAccelerationStructure->BuildBottomLevelStructure(false);
            mAccelerationStructure->AddBottomLevelInstance(structureMatrix, 0, 0);
            mAccelerationStructure->BuildTopLevelStructure(false);

            Direct3DContextManager *contextManager = mDirect3DManager->GetContextManager();
            mStructureCreationFence = contextManager->GetRayTraceContext()->Flush(contextManager->GetQueueManager(), true, false);
            mShouldBuildAccelerationStructure = false;
        }
    }
}