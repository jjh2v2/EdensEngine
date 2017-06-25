#pragma once
#include "Render/DirectX/Direct3DManager.h"
#include "Render/Texture/TextureManager.h"
#include "Render/Texture/Sampler/SamplerManager.h"
#include "Render/Shader/ShaderManager.h"
#include "Render/Mesh/MeshManager.h"

class GraphicsManager
{
public:
	GraphicsManager();
	~GraphicsManager();

	void InitializeGraphicsResources();
	void FinalizeGraphicsForRemoval();
    void Update(float deltaTime);

	Direct3DManager *GetDirect3DManager() { return mDirect3DManager; }
	TextureManager *GetTextureManager() { return mTextureManager; }
	SamplerManager *GetSamplerManager() { return mSamplerManager; }
	ShaderManager *GetShaderManager() { return mShaderManager; }
	MeshManager *GetMeshManager() { return mMeshManager; }

private:
	Direct3DManager *mDirect3DManager;
	TextureManager *mTextureManager;
	SamplerManager *mSamplerManager;
	ShaderManager *mShaderManager;
	MeshManager *mMeshManager;
};