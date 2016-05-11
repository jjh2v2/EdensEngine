#include "Render/Graphics/GraphicsManager.h"

GraphicsManager::GraphicsManager()
{
	mDirect3DManager = new Direct3DManager();
}

GraphicsManager::~GraphicsManager()
{
	delete mDirect3DManager;
}