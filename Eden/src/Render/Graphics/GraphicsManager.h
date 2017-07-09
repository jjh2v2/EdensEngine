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
    void Update(float deltaTime);
    void FlushAllAndWaitForIdle();

	Direct3DManager *GetDirect3DManager() { return mDirect3DManager; }
	TextureManager *GetTextureManager() { return mTextureManager; }
	SamplerManager *GetSamplerManager() { return mSamplerManager; }
	ShaderManager *GetShaderManager() { return mShaderManager; }
	MeshManager *GetMeshManager() { return mMeshManager; }

private:
    class UploadUpdateBackgroundJob : public Job
    {
    public:
        UploadUpdateBackgroundJob(Direct3DContextManager *contextManager);
        virtual void Execute();

    private:
        Direct3DContextManager *mContextManager;
    };

	Direct3DManager *mDirect3DManager;
	TextureManager *mTextureManager;
	SamplerManager *mSamplerManager;
	ShaderManager *mShaderManager;
	MeshManager *mMeshManager;

    UploadUpdateBackgroundJob *mBackgroundJob;
    JobBatch *mBackgroundUpdateJobBatch;
};