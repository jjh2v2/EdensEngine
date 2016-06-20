#include "Render/Graphics/GraphicsManager.h"

GraphicsManager::GraphicsManager()
{
	mDirect3DManager = new Direct3DManager();
	mTextureManager = NULL;
}

GraphicsManager::~GraphicsManager()
{
	delete mTextureManager;
	delete mDirect3DManager;
}

void GraphicsManager::InitializeGraphicsResources()
{
	mTextureManager = new TextureManager(mDirect3DManager);
	mTextureManager->LoadAllTextures();
}