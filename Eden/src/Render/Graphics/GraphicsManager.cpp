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