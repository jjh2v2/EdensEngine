#include "Render/Graphics/GraphicsManager.h"

GraphicsManager::GraphicsManager()
{
	mDirect3DManager = new Direct3DManager();
	mTextureManager = NULL;
}

GraphicsManager::~GraphicsManager()
{
	delete mDirect3DManager;
}

void GraphicsManager::InitializeGraphicsResources()
{
	mTextureManager = new TextureManager();
	//mTextureManager->LoadAllTextures();
}