#pragma once
#include "Render/DirectX/Direct3DManager.h"
#include "Render/Texture/TextureManager.h"
#include "Render/Shader/ShaderManager.h"
#include "Render/Mesh/MeshManager.h"

class GraphicsManager
{
public:
	GraphicsManager();
	~GraphicsManager();

	void InitializeGraphicsResources();

	Direct3DManager *GetDirect3DManager() { return mDirect3DManager; }

private:
	Direct3DManager *mDirect3DManager;
	TextureManager *mTextureManager;
	ShaderManager *mShaderManager;
	MeshManager *mMeshManager;
};