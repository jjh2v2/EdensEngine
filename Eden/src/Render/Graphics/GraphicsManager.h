#pragma once
#include "Render/DirectX/Direct3DManager.h"
#include "Render/Texture/TextureManager.h"
#include "Render/Texture/Sampler/SamplerManager.h"
#include "Render/Shader/ShaderManager.h"
#include "Render/Mesh/MeshManager.h"
#include "Render/RayTrace/RayTraceManager.h"

class GraphicsManager
{
public:
	GraphicsManager();
	~GraphicsManager();

	void InitializeGraphicsResources();
    void Update(float deltaTime, Camera *camera);
    void FlushAllAndWaitForIdle();

	Direct3DManager *GetDirect3DManager() { return mDirect3DManager; }
	TextureManager *GetTextureManager() { return mTextureManager; }
	SamplerManager *GetSamplerManager() { return mSamplerManager; }
	ShaderManager *GetShaderManager() { return mShaderManager; }
	MeshManager *GetMeshManager() { return mMeshManager; }
    RayTraceManager *GetRayTraceManager() { return mRayTraceManager; }

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
    RayTraceManager *mRayTraceManager;

    UploadUpdateBackgroundJob *mBackgroundJob;
    JobBatch *mBackgroundUpdateJobBatch;
};