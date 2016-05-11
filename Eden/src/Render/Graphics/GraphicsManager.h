#pragma once
#include "Render/DirectX/Direct3DManager.h"

class GraphicsManager
{
public:
	GraphicsManager();
	~GraphicsManager();

	Direct3DManager *GetDirect3DManager() { return mDirect3DManager; }

private:
	Direct3DManager *mDirect3DManager;
};