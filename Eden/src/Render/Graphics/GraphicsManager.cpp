#include "Render/Graphics/GraphicsManager.h"

GraphicsManager::GraphicsManager()
{
	mDirect3DManager = new Direct3DManager();
	mShaderManager = NULL;
	mTextureManager = NULL;
	mMeshManager = NULL;
}

GraphicsManager::~GraphicsManager()
{
	delete mMeshManager;
	delete mShaderManager;
	delete mSamplerManager;
	delete mTextureManager;
	delete mDirect3DManager;
}

void GraphicsManager::InitializeGraphicsResources()
{
	mTextureManager = new TextureManager(mDirect3DManager);
	mTextureManager->LoadAllTextures();
	mSamplerManager = new SamplerManager(mDirect3DManager);
	mShaderManager = new ShaderManager(mDirect3DManager->GetDevice());
	mMeshManager = new MeshManager();
	mMeshManager->LoadAllMeshes(mDirect3DManager);
}

void GraphicsManager::FinalizeGraphicsForRemoval()
{
	//need to flush everything going on before we start destroying things
	Direct3DContextManager *contextManager = mDirect3DManager->GetContextManager();
	contextManager->GetGraphicsContext()->Flush(contextManager->GetQueueManager(), true);
}