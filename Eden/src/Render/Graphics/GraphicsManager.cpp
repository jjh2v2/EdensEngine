#include "Render/Graphics/GraphicsManager.h"
#include "Core/Threading/ThreadPoolManager.h"

GraphicsManager::GraphicsManager()
{
	mDirect3DManager = new Direct3DManager();
	mShaderManager = NULL;
	mTextureManager = NULL;
	mMeshManager = NULL;
    mRayTraceManager = NULL;

    mBackgroundJob = new UploadUpdateBackgroundJob(mDirect3DManager->GetContextManager());
    mBackgroundUpdateJobBatch = new JobBatch();
}

GraphicsManager::~GraphicsManager()
{
    mBackgroundUpdateJobBatch->WaitForBatch();
    delete mBackgroundUpdateJobBatch;
    delete mBackgroundJob;

    delete mRayTraceManager;
	delete mMeshManager;
	delete mShaderManager;
	delete mSamplerManager;
	delete mTextureManager;
	delete mDirect3DManager;
}

void GraphicsManager::InitializeGraphicsResources()
{
	mTextureManager = new TextureManager(mDirect3DManager);
	mTextureManager->LoadTextureManifest();
	mSamplerManager = new SamplerManager(mDirect3DManager);
	mShaderManager = new ShaderManager(mDirect3DManager->GetDevice());
	mMeshManager = new MeshManager();
	mMeshManager->LoadAllMeshes(mDirect3DManager);
    mRayTraceManager = new RayTraceManager(mDirect3DManager);
    mRayTraceManager->QueueRayTraceAccelerationStructureCreation();
}

GraphicsManager::UploadUpdateBackgroundJob::UploadUpdateBackgroundJob(Direct3DContextManager *contextManager)
{
    mContextManager = contextManager;
}

void GraphicsManager::UploadUpdateBackgroundJob::Execute()
{
    mContextManager->GetUploadContext()->ProcessBackgroundUploads(MAX_UPLOADS_PROCESSED_PER_BATCH);
    mContextManager->GetUploadContext()->ProcessFinishedUploads(mContextManager->GetQueueManager()->GetCopyQueue()->PollCurrentFenceValue());
}

void GraphicsManager::Update(float deltaTime)
{
    if (mBackgroundUpdateJobBatch->IsCompleted())
    {
        Application::Assert(mBackgroundUpdateJobBatch->GetNumJobs() == 0);

        mBackgroundUpdateJobBatch->AddBatchJob(mBackgroundJob);
        ThreadPoolManager::GetSingleton()->GetBackgroundThreadPool()->AddJobBatch(mBackgroundUpdateJobBatch);
    }

    mTextureManager->ProcessCurrentUploads();
    mTextureManager->ProcessCurrentComputeWork();
    mRayTraceManager->Update();
    mDirect3DManager->GetContextManager()->GetGraphicsContext()->FlushDeferredTransitions();
}

void GraphicsManager::FlushAllAndWaitForIdle()
{
    mDirect3DManager->GetContextManager()->FinishContextsAndWaitForIdle();
}