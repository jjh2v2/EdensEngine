#include "Render/Renderer/DeferredRenderer.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"
#include "Render/Shadow/SDSMShadowManager.h"
#include "Render/PostProcess/PostProcessManager.h"
#include "Util/Math/MatrixHelper.h"
#include "Render/Material/Material.h"

DeferredRenderer::DeferredRenderer(GraphicsManager *graphicsManager)
{
	mGraphicsManager = graphicsManager;
    mPreviousFrameFence = 0;
    mMeshesAddedToRayTrace = false;
    mShowRayTrace = false;

	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	Direct3DContextManager *contextManager = direct3DManager->GetContextManager();
	contextManager->GetGraphicsContext()->Flush(contextManager->GetQueueManager(), true);
	
	CreateTargets(direct3DManager->GetScreenSize());
	
    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mCameraConstantBuffer[i] = contextManager->CreateConstantBuffer(sizeof(CameraBuffer));
        mLightBuffer[i] = contextManager->CreateConstantBuffer(sizeof(LightingMainBuffer));
        mSkyBuffer[i] = contextManager->CreateConstantBuffer(sizeof(SkyBuffer));
        mRayShadowBlurBuffer[i] = contextManager->CreateConstantBuffer(sizeof(RayShadowBlurBuffer));
    }

	{
        ConstantBuffer *materialConstantBuffers[FRAME_BUFFER_COUNT];
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            materialConstantBuffers[i] = contextManager->CreateConstantBuffer(sizeof(MaterialConstants));
        }

        ConstantBuffer *shadowConstantBuffers[FRAME_BUFFER_COUNT];
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            shadowConstantBuffers[i] = contextManager->CreateConstantBuffer(sizeof(D3DXMATRIX));
        }

        //TDA implement material manager
		Material *newMaterial = new Material(materialConstantBuffers);
        ShadowMaterial *shadowMaterial = new ShadowMaterial(shadowConstantBuffers);

		newMaterial->SetTexture(MaterialTextureType_Diffuse, mGraphicsManager->GetTextureManager()->GetTexture("MageDiffuseFire"));
		newMaterial->SetTexture(MaterialTextureType_Normal, mGraphicsManager->GetTextureManager()->GetTexture("MageNormal"));
		newMaterial->SetTexture(MaterialTextureType_Roughmetal, mGraphicsManager->GetTextureManager()->GetTexture("MageRoughMetal"));
		newMaterial->GetMaterialBuffer()->SetUsesNormalMap(true);
		newMaterial->GetMaterialBuffer()->SetUsesRoughmetalMap(true);
		mSceneEntity = new SceneEntity(mGraphicsManager->GetMeshManager()->GetMesh("MageBiNormals"), newMaterial, shadowMaterial);
		mSceneEntity->SetPosition(Vector3(0, -5.0f, 20.0f));
		mSceneEntity->SetRotation(Vector3(0, MathHelper::Radian() * 90.0f, 0));
	}
	{
        ConstantBuffer *materialConstantBuffers[FRAME_BUFFER_COUNT];
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            materialConstantBuffers[i] = contextManager->CreateConstantBuffer(sizeof(MaterialConstants));
        }

        ConstantBuffer *shadowConstantBuffers[FRAME_BUFFER_COUNT];
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            shadowConstantBuffers[i] = contextManager->CreateConstantBuffer(sizeof(D3DXMATRIX));
        }

		Material *newMaterial = new Material(materialConstantBuffers);
        ShadowMaterial *shadowMaterial = new ShadowMaterial(shadowConstantBuffers);

		newMaterial->SetTexture(MaterialTextureType_Diffuse, mGraphicsManager->GetTextureManager()->GetTexture("MageDiffuseFire"));
		newMaterial->SetTexture(MaterialTextureType_Normal, mGraphicsManager->GetTextureManager()->GetTexture("MageNormal"));
		newMaterial->SetTexture(MaterialTextureType_Roughmetal, mGraphicsManager->GetTextureManager()->GetTexture("MageRoughMetal"));
		newMaterial->GetMaterialBuffer()->SetUsesNormalMap(true);
		newMaterial->GetMaterialBuffer()->SetUsesRoughmetalMap(true);
		mSceneEntity2 = new SceneEntity(mGraphicsManager->GetMeshManager()->GetMesh("MageBiNormals"), newMaterial, shadowMaterial);
		mSceneEntity2->SetPosition(Vector3(10, -5.0f, 20.0f));
		mSceneEntity2->SetRotation(Vector3(0, MathHelper::Radian() * 180.0f, 0));
	}
	{
        ConstantBuffer *materialConstantBuffers[FRAME_BUFFER_COUNT];
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            materialConstantBuffers[i] = contextManager->CreateConstantBuffer(sizeof(MaterialConstants));
        }

        ConstantBuffer *shadowConstantBuffers[FRAME_BUFFER_COUNT];
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            shadowConstantBuffers[i] = contextManager->CreateConstantBuffer(sizeof(D3DXMATRIX));
        }

		Material *newMaterial = new Material(materialConstantBuffers);
        ShadowMaterial *shadowMaterial = new ShadowMaterial(shadowConstantBuffers);

		newMaterial->SetTexture(MaterialTextureType_Diffuse, mGraphicsManager->GetTextureManager()->GetTexture("MageDiffuseFire"));
		newMaterial->SetTexture(MaterialTextureType_Normal, mGraphicsManager->GetTextureManager()->GetTexture("MageNormal"));
		newMaterial->SetTexture(MaterialTextureType_Roughmetal, mGraphicsManager->GetTextureManager()->GetTexture("MageRoughMetal"));
		newMaterial->GetMaterialBuffer()->SetUsesNormalMap(true);
		newMaterial->GetMaterialBuffer()->SetUsesRoughmetalMap(true);
		mSceneEntity3 = new SceneEntity(mGraphicsManager->GetMeshManager()->GetMesh("MageBiNormals"), newMaterial, shadowMaterial);
		mSceneEntity3->SetPosition(Vector3(-10, -5.0f, 20.0f));
		mSceneEntity3->SetRotation(Vector3(0, MathHelper::Radian() * 180.0f, 0));
	}
    {
        ConstantBuffer *materialConstantBuffers[FRAME_BUFFER_COUNT];
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            materialConstantBuffers[i] = contextManager->CreateConstantBuffer(sizeof(MaterialConstants));
        }

        ConstantBuffer *shadowConstantBuffers[FRAME_BUFFER_COUNT];
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            shadowConstantBuffers[i] = contextManager->CreateConstantBuffer(sizeof(D3DXMATRIX));
        }

        Material *newMaterial = new Material(materialConstantBuffers);
        ShadowMaterial *shadowMaterial = new ShadowMaterial(shadowConstantBuffers);

        newMaterial->SetTexture(MaterialTextureType_Diffuse, mGraphicsManager->GetTextureManager()->GetTexture("DefaultPurple"));
        newMaterial->GetMaterialBuffer()->SetUsesNormalMap(false);
        newMaterial->GetMaterialBuffer()->SetUsesRoughmetalMap(false);
        mSceneEntity4 = new SceneEntity(mGraphicsManager->GetMeshManager()->GetMesh("Cube"), newMaterial, shadowMaterial);
        mSceneEntity4->SetPosition(Vector3(0.0f, -5.0f, 20.0f));
        mSceneEntity4->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
        mSceneEntity4->SetScale(Vector3(25.0f, 0.4f, 25.0f));
    }

    mSkyTexture = mGraphicsManager->GetTextureManager()->GetTexture("SnowyMountains");
    mSkyMesh = mGraphicsManager->GetMeshManager()->GetMesh("Sphere");

    ShaderPipelinePermutation emptyComputePermutation;
    ShaderPSO *envFilterShader = mGraphicsManager->GetShaderManager()->GetShader("FilterEnvironmentMap", emptyComputePermutation);
    mFilteredCubeMap = mGraphicsManager->GetTextureManager()->FilterCubeMap(mSkyTexture, mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_POINT_WRAP), envFilterShader);

    ShaderPSO *generateEnvLookupShader = mGraphicsManager->GetShaderManager()->GetShader("GenerateEnvironmentMapLookup", emptyComputePermutation);
    mEnvironmentMapLookup = mGraphicsManager->GetTextureManager()->GenerateTexture(generateEnvLookupShader, 256, 256);

    mShadowManager = new SDSMShadowManager(mGraphicsManager);
    mPostProcessManager = new PostProcessManager(mGraphicsManager);

    mRayTraceManager = NULL;
    if (direct3DManager->IsDXRSupported())
    {
        mRayTraceManager = new RayTraceManager(direct3DManager, mGraphicsManager->GetShaderManager()->GetRootSignatureManager());
    }
}

DeferredRenderer::~DeferredRenderer()
{
    delete mRayTraceManager;
    delete mPostProcessManager;
    delete mShadowManager;

	FreeTargets();

	Direct3DContextManager *contextManager = mGraphicsManager->GetDirect3DManager()->GetContextManager();

    contextManager->FreeRenderTarget(mEnvironmentMapLookup);
    contextManager->FreeFilteredCubeMap(mFilteredCubeMap);

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        contextManager->FreeConstantBuffer(mCameraConstantBuffer[i]);
        mCameraConstantBuffer[i] = NULL;

        contextManager->FreeConstantBuffer(mLightBuffer[i]);
        mLightBuffer[i] = NULL;

        contextManager->FreeConstantBuffer(mSkyBuffer[i]);
        mSkyBuffer[i] = NULL;

        contextManager->FreeConstantBuffer(mRayShadowBlurBuffer[i]);
        mRayShadowBlurBuffer[i] = NULL;
    }
	
	{
		Material *material = mSceneEntity->GetMaterial();
        ShadowMaterial *shadowMaterial = mSceneEntity->GetShadowMaterial();
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            contextManager->FreeConstantBuffer(material->GetConstantBuffer(i));
            contextManager->FreeConstantBuffer(shadowMaterial->GetConstantBuffer(i));
        }
		delete material;
        delete shadowMaterial;
		delete mSceneEntity;
	}
	{
		Material *material = mSceneEntity2->GetMaterial();
        ShadowMaterial *shadowMaterial = mSceneEntity2->GetShadowMaterial();
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            contextManager->FreeConstantBuffer(material->GetConstantBuffer(i));
            contextManager->FreeConstantBuffer(shadowMaterial->GetConstantBuffer(i));
        }
		delete material;
        delete shadowMaterial;
		delete mSceneEntity2;
	}
	{
		Material *material = mSceneEntity3->GetMaterial();
        ShadowMaterial *shadowMaterial = mSceneEntity3->GetShadowMaterial();
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            contextManager->FreeConstantBuffer(material->GetConstantBuffer(i));
            contextManager->FreeConstantBuffer(shadowMaterial->GetConstantBuffer(i));
        }
		delete material;
        delete shadowMaterial;
		delete mSceneEntity3;
	}
    {
        Material *material = mSceneEntity4->GetMaterial();
        ShadowMaterial *shadowMaterial = mSceneEntity4->GetShadowMaterial();
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            contextManager->FreeConstantBuffer(material->GetConstantBuffer(i));
            contextManager->FreeConstantBuffer(shadowMaterial->GetConstantBuffer(i));
        }
        delete material;
        delete shadowMaterial;
        delete mSceneEntity4;
    }
}

void DeferredRenderer::FreeTargets()
{
	Direct3DContextManager *contextManager = mGraphicsManager->GetDirect3DManager()->GetContextManager();

	for (uint32 targetIndex = 0; targetIndex < mGBufferTargets.CurrentSize(); targetIndex++)
	{
		contextManager->FreeRenderTarget(mGBufferTargets[targetIndex]);
	}

	mGBufferTargets.Clear();

	if (mGBufferDepth)
	{
		contextManager->FreeDepthStencilTarget(mGBufferDepth);
		mGBufferDepth = NULL;
	}

    contextManager->FreeRenderTarget(mHDRTarget);
    contextManager->FreeRenderTarget(mRayShadowBlurTarget);
}

void DeferredRenderer::RenderRayTracing()
{
    if (mRayTraceManager)
    {
        if (!mMeshesAddedToRayTrace && mGraphicsManager->GetMeshManager()->GetMesh("MageBiNormals")->IsReady() && mGraphicsManager->GetMeshManager()->GetMesh("Cube")->IsReady())
        {
            mMeshesAddedToRayTrace = true;
            D3DXMATRIX transform1 = mSceneEntity->CalculateTransform();
            mRayTraceManager->AddMeshToAccelerationStructure(mGraphicsManager->GetMeshManager()->GetMesh("MageBiNormals"), transform1, RayTraceAccelerationStructureType_Fastest_Trace);

            D3DXMATRIX transform2 = mSceneEntity2->CalculateTransform();
            mRayTraceManager->AddMeshToAccelerationStructure(mGraphicsManager->GetMeshManager()->GetMesh("MageBiNormals"), transform2, RayTraceAccelerationStructureType_Fastest_Trace);

            D3DXMATRIX transform3 = mSceneEntity3->CalculateTransform();
            mRayTraceManager->AddMeshToAccelerationStructure(mGraphicsManager->GetMeshManager()->GetMesh("MageBiNormals"), transform3, RayTraceAccelerationStructureType_Fastest_Trace);
        
            D3DXMATRIX transform4 = mSceneEntity4->CalculateTransform();
            mRayTraceManager->AddMeshToAccelerationStructure(mGraphicsManager->GetMeshManager()->GetMesh("Cube"), transform4, RayTraceAccelerationStructureType_Fastest_Trace);
        }

        mRayTraceManager->Update(mActiveScene->GetMainCamera(), mActiveScene->GetSunLight()->GetDirection() * -1.0f);
        mRayTraceManager->RenderShadowRayTrace(mGBufferDepth);
    }
}

void DeferredRenderer::ProcessRayShadows()
{
    if (mRayTraceManager)
    {
        Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
        GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
        Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();
        Vector2 screenSize = direct3DManager->GetScreenSize();

        graphicsContext->InsertPixBeginEvent(0xFFFF0000, "Ray Shadow Blur");

        graphicsContext->TransitionResource(mRayTraceManager->GetRenderTarget(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
        graphicsContext->TransitionResource(mRayShadowBlurTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

        const uint32 testNumSRVsNeeded = 10;
        const uint32 testNumSamplersNeeded = 1;
        RenderPassDescriptorHeap *shadowSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_RayShadow, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), testNumSRVsNeeded);
        RenderPassDescriptorHeap *shadowSamplerDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_RayShadow, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, direct3DManager->GetFrameIndex(), testNumSamplersNeeded);

        graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, shadowSRVDescHeap->GetHeap());
        graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, shadowSamplerDescHeap->GetHeap());

        graphicsContext->SetViewport(direct3DManager->GetScreenViewport());
        graphicsContext->SetScissorRect(0, 0, (uint32)screenSize.X, (uint32)screenSize.Y);

        RayShadowBlurBuffer shadowBlurBuffer;
        shadowBlurBuffer.oneOverScreenSize = Vector2(1.0f / screenSize.X, 1.0f / screenSize.Y);
        shadowBlurBuffer.blurAmount = 3.0f;
        mRayShadowBlurBuffer[direct3DManager->GetFrameIndex()]->SetConstantBufferData(&shadowBlurBuffer, sizeof(RayShadowBlurBuffer));

        Sampler *linearSampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_LINEAR_CLAMP);

        DescriptorHeapHandle shadowBlurSRVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
        DescriptorHeapHandle shadowBlurSampleHandle = shadowSamplerDescHeap->GetHeapHandleBlock(1);
        DescriptorHeapHandle shadowBlurCBVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);

        direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowBlurSRVHandle.GetCPUHandle(), mRayTraceManager->GetRenderTarget()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowBlurSampleHandle.GetCPUHandle(), linearSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowBlurCBVHandle.GetCPUHandle(), mRayShadowBlurBuffer[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        graphicsContext->SetRenderTarget(mRayShadowBlurTarget->GetRenderTargetViewHandle().GetCPUHandle());

        ShaderPipelinePermutation rayShadowShaderPermutation(Render_Standard_NoDepth, Target_Single_R32_NoDepth, InputLayout_Standard);
        ShaderPSO *shadowBlurShader = mGraphicsManager->GetShaderManager()->GetShader("RayShadowBlur", rayShadowShaderPermutation);
        graphicsContext->SetPipelineState(shadowBlurShader);
        graphicsContext->SetRootSignature(shadowBlurShader->GetRootSignature(), NULL);

        graphicsContext->SetGraphicsDescriptorTable(0, shadowBlurSRVHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(1, shadowBlurSampleHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(2, shadowBlurCBVHandle.GetGPUHandle());

        graphicsContext->DrawFullScreenTriangle();

        graphicsContext->InsertPixEndEvent();
    }
}

void DeferredRenderer::CreateTargets(Vector2 screenSize)
{
	Direct3DContextManager *contextManager = mGraphicsManager->GetDirect3DManager()->GetContextManager();

	RenderTarget *gbufferAlbedoTarget = contextManager->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
	RenderTarget *gbufferNormalsTarget = contextManager->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
	RenderTarget *gbufferMaterialTarget = contextManager->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
	mGBufferTargets.Add(gbufferAlbedoTarget);
	mGBufferTargets.Add(gbufferNormalsTarget);
	mGBufferTargets.Add(gbufferMaterialTarget);

	mGBufferDepth = contextManager->CreateDepthStencilTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_D32_FLOAT_S8X24_UINT, 1, 1, 0);

    mHDRTarget = contextManager->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
    mRayShadowBlurTarget = contextManager->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R32_FLOAT, false, 1, 1, 0);
}

void DeferredRenderer::OnScreenChanged(Vector2 screenSize)
{
	FreeTargets();
	CreateTargets(screenSize);
}

void DeferredRenderer::ClearFrameBuffers()
{
	GraphicsContext *graphicsContext = mGraphicsManager->GetDirect3DManager()->GetContextManager()->GetGraphicsContext(1);
	float blackColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    graphicsContext->InsertPixBeginEvent(0xFFFFFFFF, "Clear Buffers");

	graphicsContext->TransitionResource(mGBufferTargets[0], D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	graphicsContext->TransitionResource(mGBufferTargets[1], D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	graphicsContext->TransitionResource(mGBufferTargets[2], D3D12_RESOURCE_STATE_RENDER_TARGET, false);
    graphicsContext->TransitionResource(mHDRTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
    graphicsContext->TransitionResource(mGBufferDepth, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

	graphicsContext->ClearRenderTarget(mGBufferTargets[0]->GetRenderTargetViewHandle().GetCPUHandle(), blackColor);
	graphicsContext->ClearRenderTarget(mGBufferTargets[1]->GetRenderTargetViewHandle().GetCPUHandle(), blackColor);
	graphicsContext->ClearRenderTarget(mGBufferTargets[2]->GetRenderTargetViewHandle().GetCPUHandle(), blackColor);
    graphicsContext->ClearRenderTarget(mHDRTarget->GetRenderTargetViewHandle().GetCPUHandle(), blackColor);
	graphicsContext->ClearDepthStencilTarget(mGBufferDepth->GetDepthStencilViewHandle().GetCPUHandle(), 0.0f, 0);

    graphicsContext->InsertPixEndEvent();
}

void DeferredRenderer::RenderGBuffer()
{
	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext(1);
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();
	Vector2 screenSize = direct3DManager->GetScreenSize();

    graphicsContext->InsertPixBeginEvent(0xFFFF0000, "GBuffer Pass");

    const uint32 testNumSRVsNeeded = 20; //TDA this needs to be changed to scene entity 
    const uint32 testNumSamplersNeeded = 3; //TDA this needs to be changed to some definition
    RenderPassDescriptorHeap *gbufferSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_GBuffer, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), testNumSRVsNeeded);
    RenderPassDescriptorHeap *gbufferSamplerDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_GBuffer, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, direct3DManager->GetFrameIndex(), testNumSamplersNeeded);

    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, gbufferSRVDescHeap->GetHeap());
    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, gbufferSamplerDescHeap->GetHeap());

	graphicsContext->SetViewport(direct3DManager->GetScreenViewport());
	graphicsContext->SetScissorRect(0, 0, (uint32)screenSize.X, (uint32)screenSize.Y);

	// set up the default samplers
	DescriptorHeapHandle gBufferSamplersHandle = gbufferSamplerDescHeap->GetHeapHandleBlock(3);
	D3D12_CPU_DESCRIPTOR_HANDLE gBufferSamplersHandleOffset = gBufferSamplersHandle.GetCPUHandle();
	Sampler *defaultAnisoSampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_ANISO);

	for (uint32 i = 0; i < GBufferTextureInputCount; i++)
	{
		direct3DManager->GetDevice()->CopyDescriptorsSimple(1, gBufferSamplersHandleOffset, defaultAnisoSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		gBufferSamplersHandleOffset.ptr += gbufferSamplerDescHeap->GetDescriptorSize();
	}

	DescriptorHeapHandle perFrameCameraHandle = gbufferSRVDescHeap->GetHeapHandleBlock(1);
	direct3DManager->GetDevice()->CopyDescriptorsSimple(1, perFrameCameraHandle.GetCPUHandle(), mCameraConstantBuffer[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//set up the render targets
	D3D12_CPU_DESCRIPTOR_HANDLE gbufferRtvHandles[3] = {
		mGBufferTargets[0]->GetRenderTargetViewHandle().GetCPUHandle(),
		mGBufferTargets[1]->GetRenderTargetViewHandle().GetCPUHandle(),
		mGBufferTargets[2]->GetRenderTargetViewHandle().GetCPUHandle()
	};
	graphicsContext->SetRenderTargets(3, gbufferRtvHandles, mGBufferDepth->GetDepthStencilViewHandle().GetCPUHandle());

	//shader setup
	ShaderPipelinePermutation permutation(Render_Standard, Target_GBuffer, InputLayout_Standard);
	ShaderPSO *shaderPSO = mGraphicsManager->GetShaderManager()->GetShader("GBufferLit", permutation);
	graphicsContext->SetPipelineState(shaderPSO);
	graphicsContext->SetRootSignature(shaderPSO->GetRootSignature(), NULL);

	graphicsContext->SetGraphicsDescriptorTable(2, perFrameCameraHandle.GetGPUHandle());
	graphicsContext->SetGraphicsDescriptorTable(3, gBufferSamplersHandle.GetGPUHandle());

	DynamicArray<MaterialTextureType> textureTypes;
	textureTypes.Add(MaterialTextureType_Diffuse);
	textureTypes.Add(MaterialTextureType_Normal);
	textureTypes.Add(MaterialTextureType_Roughmetal);

	RenderPassContext renderPassContext(graphicsContext, gbufferSRVDescHeap, NULL, textureTypes, direct3DManager->GetFrameIndex());
	mSceneEntity->Render(&renderPassContext);
	mSceneEntity2->Render(&renderPassContext);
	mSceneEntity3->Render(&renderPassContext);
    mSceneEntity4->Render(&renderPassContext);

    graphicsContext->TransitionResource(mGBufferDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);
    graphicsContext->InsertPixEndEvent();
}

void DeferredRenderer::RenderSky()
{
    if (!mSkyTexture || !mSkyTexture->GetIsReady() || !mSkyMesh || !mSkyMesh->IsReady())
    {
        return;
    }

    Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();
    Vector2 screenSize = direct3DManager->GetScreenSize();

    graphicsContext->InsertPixBeginEvent(0xFF0000FF, "Sky Pass");

    const uint32 numSRVsNeeded = 3;
    const uint32 numSamplersNeeded = 1;
    RenderPassDescriptorHeap *skyBoxSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_Sky, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), numSRVsNeeded);
    RenderPassDescriptorHeap *skyBoxSamplerDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_Sky, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, direct3DManager->GetFrameIndex(), numSamplersNeeded);

    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, skyBoxSRVDescHeap->GetHeap());
    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, skyBoxSamplerDescHeap->GetHeap());

    graphicsContext->SetViewport(direct3DManager->GetScreenViewport());
    graphicsContext->SetScissorRect(0, 0, (uint32)screenSize.X, (uint32)screenSize.Y);

    DescriptorHeapHandle skyTexturesHandle = skyBoxSRVDescHeap->GetHeapHandleBlock(2);
    D3D12_CPU_DESCRIPTOR_HANDLE skyTexturesHandleOffset = skyTexturesHandle.GetCPUHandle();

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, skyTexturesHandleOffset, mSkyTexture->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    skyTexturesHandleOffset.ptr += skyBoxSRVDescHeap->GetDescriptorSize();
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, skyTexturesHandleOffset, mSkyTexture->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    Sampler *skySampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_LINEAR_CLAMP);
    DescriptorHeapHandle skySamplerHandle = skyBoxSamplerDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, skySamplerHandle.GetCPUHandle(), skySampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    Camera *mainCamera = mActiveScene->GetMainCamera();
    D3DXMATRIX positionMatrix;
    Vector3 cameraPosition = mainCamera->GetPosition();
    D3DXMatrixTranslation(&positionMatrix, cameraPosition.X, cameraPosition.Y, cameraPosition.Z);
    D3DXMATRIX worldViewProjMatrix = positionMatrix * mainCamera->GetViewMatrix() * mainCamera->GetReverseProjectionMatrix();
    D3DXMatrixTranspose(&worldViewProjMatrix, &worldViewProjMatrix);

    SkyBuffer skyBufferData;
    skyBufferData.wvpMatrix = worldViewProjMatrix;
    skyBufferData.fade = 0;
    mSkyBuffer[direct3DManager->GetFrameIndex()]->SetConstantBufferData(&skyBufferData, sizeof(SkyBuffer));

    DescriptorHeapHandle skyBufferHandle = skyBoxSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, skyBufferHandle.GetCPUHandle(), mSkyBuffer[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetRenderTarget(mHDRTarget->GetRenderTargetViewHandle().GetCPUHandle());

    ShaderPipelinePermutation skyShaderPermutation(Render_Sky, Target_Single_16_NoDepth, InputLayout_Standard);
    ShaderPSO *skyBoxShader = mGraphicsManager->GetShaderManager()->GetShader("SkyBox", skyShaderPermutation);
    graphicsContext->SetPipelineState(skyBoxShader);
    graphicsContext->SetRootSignature(skyBoxShader->GetRootSignature(), NULL);

    graphicsContext->SetGraphicsDescriptorTable(0, skyTexturesHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, skySamplerHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(2, skyBufferHandle.GetGPUHandle());

    graphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    graphicsContext->SetVertexBuffer(0, mSkyMesh->GetVertexBuffer());
    graphicsContext->SetIndexBuffer(mSkyMesh->GetIndexBuffer());
    graphicsContext->Draw(mSkyMesh->GetVertexCount());

    graphicsContext->InsertPixEndEvent();
}

void DeferredRenderer::RenderShadows(D3DXMATRIX &lightViewMatrix, D3DXMATRIX &lightProjMatrix)
{
    mShadowManager->ComputeShadowPartitions(mActiveScene->GetMainCamera(), lightViewMatrix, lightProjMatrix, mGBufferDepth);
    
    D3DXMATRIX lightViewProjMatrix = lightViewMatrix * lightProjMatrix;
    DynamicArray<SceneEntity*> sceneEntities;
    sceneEntities.Add(mSceneEntity);
    sceneEntities.Add(mSceneEntity2);
    sceneEntities.Add(mSceneEntity3);
    sceneEntities.Add(mSceneEntity4);

    mShadowManager->RenderShadowMapPartitions(lightViewProjMatrix, sceneEntities);
}

void DeferredRenderer::RenderLightingMain(const D3DXMATRIX &viewMatrix, const D3DXMATRIX &projectionMatrix, const D3DXMATRIX &viewToLightProjMatrix, const D3DXMATRIX &viewInvMatrix)
{
    if (!mFilteredCubeMap || !mFilteredCubeMap->GetIsReady() || !mEnvironmentMapLookup || !mEnvironmentMapLookup->GetIsReady())
    {
        //early out if our environment lighting isn't ready
        return;
    }

    Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();
    Vector2 screenSize = direct3DManager->GetScreenSize();

    graphicsContext->InsertPixBeginEvent(0xFFFFFF00, "Lighting Pass");

    const uint32 numSRVsNeeded = 12;
    const uint32 numSamplersNeeded = 3; 
    RenderPassDescriptorHeap *lightingSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_Lighting, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), numSRVsNeeded);
    RenderPassDescriptorHeap *lightingSamplerDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_Lighting, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, direct3DManager->GetFrameIndex(), numSamplersNeeded);

    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, lightingSRVDescHeap->GetHeap());
    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, lightingSamplerDescHeap->GetHeap());

    graphicsContext->SetViewport(direct3DManager->GetScreenViewport());
    graphicsContext->SetScissorRect(0, 0, (uint32)screenSize.X, (uint32)screenSize.Y);

    DescriptorHeapHandle lightingSRVsHandle = lightingSRVDescHeap->GetHeapHandleBlock(numSRVsNeeded + 1);
    D3D12_CPU_DESCRIPTOR_HANDLE lightingSRVHandleOffset = lightingSRVsHandle.GetCPUHandle();
    uint32 lightingSRVHandleOffsetIncrement = lightingSRVDescHeap->GetDescriptorSize();

    DescriptorHeapHandle lightingSamplersHandle = lightingSamplerDescHeap->GetHeapHandleBlock(2);
    D3D12_CPU_DESCRIPTOR_HANDLE lightingSamplersHandleOffset = lightingSamplersHandle.GetCPUHandle();
    uint32 lightingSamplersHandleOffsetIncrement = lightingSamplerDescHeap->GetDescriptorSize();

    for (uint32 i = 0; i < mGBufferTargets.CurrentSize(); i++)
    {
        direct3DManager->GetDevice()->CopyDescriptorsSimple(1, lightingSRVHandleOffset, mGBufferTargets[i]->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        lightingSRVHandleOffset.ptr += lightingSRVHandleOffsetIncrement;
    }
        
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, lightingSRVHandleOffset, mGBufferDepth->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    lightingSRVHandleOffset.ptr += lightingSRVHandleOffsetIncrement;

    for (uint32 i = 0; i < SDSM_SHADOW_PARTITION_COUNT; i++)
    {
        direct3DManager->GetDevice()->CopyDescriptorsSimple(1, lightingSRVHandleOffset, mShadowManager->GetShadowEVSMTexture(i)->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        lightingSRVHandleOffset.ptr += lightingSRVHandleOffsetIncrement;
    }

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, lightingSRVHandleOffset, mFilteredCubeMap->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    lightingSRVHandleOffset.ptr += lightingSRVHandleOffsetIncrement;

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, lightingSRVHandleOffset, mEnvironmentMapLookup->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    lightingSRVHandleOffset.ptr += lightingSRVHandleOffsetIncrement;

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, lightingSRVHandleOffset, mShadowManager->GetShadowPartitionBuffer()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    lightingSRVHandleOffset.ptr += lightingSRVHandleOffsetIncrement;

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, lightingSRVHandleOffset, mRayShadowBlurTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    Sampler *lightingSampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_ANISO_16_CLAMP);
    Sampler *shadowSampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_LINEAR_CLAMP);

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, lightingSamplersHandleOffset, lightingSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    lightingSamplersHandleOffset.ptr += lightingSamplersHandleOffsetIncrement;
        
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, lightingSamplersHandleOffset, lightingSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    lightingSamplersHandleOffset.ptr += lightingSamplersHandleOffsetIncrement;

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, lightingSamplersHandleOffset, shadowSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    Camera *mainCamera = mActiveScene->GetMainCamera();
    Vector3 lightDir = mActiveScene->GetSunLight()->GetDirection();

    D3DXVECTOR3 lightDirectionView;
    D3DXMATRIX viewTransposed;
    D3DXMatrixTranspose(&viewTransposed, &viewMatrix);

    D3DXVec3TransformNormal(&lightDirectionView, &mActiveScene->GetSunLight()->GetDirection().AsD3DVector3(), &viewTransposed);
    Vector4 lightDirView = Vector4(lightDirectionView.x, lightDirectionView.y, lightDirectionView.z, 1.0f);

    LightingMainBuffer lightBuffer;
    lightBuffer.viewMatrix = viewMatrix;            //all pre-transposed
    lightBuffer.projectionMatrix = projectionMatrix;
    lightBuffer.viewToLightProjMatrix = viewToLightProjMatrix;
    lightBuffer.viewInvMatrix = viewInvMatrix;
    lightBuffer.groundColor = Vector4(0.705f, 0.809f, 0.839f, 1.0f);
    lightBuffer.skyColor = Vector4(0.0f, 0.588f, 1.0f, 1.0f);
    lightBuffer.lightDir = lightDirView;
    lightBuffer.lightColor = Vector4(1.0f, 0.889f, 0.717f, 1.0f);
    lightBuffer.bufferDimensions = screenSize;
    lightBuffer.lightIntensity = 2.0f;
    lightBuffer.ambientIntensity = 0.2f;
    lightBuffer.brdfSpecular = 1.0f;
    lightBuffer.specularIBLMipLevels = 11;

    mLightBuffer[direct3DManager->GetFrameIndex()]->SetConstantBufferData(&lightBuffer, sizeof(LightingMainBuffer));

    DescriptorHeapHandle perFrameLightHandle = lightingSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, perFrameLightHandle.GetCPUHandle(), mLightBuffer[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetRenderTarget(mHDRTarget->GetRenderTargetViewHandle().GetCPUHandle());

    ShaderPipelinePermutation lightMainPermutation(Render_Standard_NoDepth, Target_Single_16_NoDepth, InputLayout_Standard);
    ShaderPSO *lightMainShader = mGraphicsManager->GetShaderManager()->GetShader("LightingMain", lightMainPermutation);
    graphicsContext->SetPipelineState(lightMainShader);
    graphicsContext->SetRootSignature(lightMainShader->GetRootSignature(), NULL);

    graphicsContext->SetGraphicsDescriptorTable(0, lightingSRVsHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, lightingSamplersHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(2, perFrameLightHandle.GetGPUHandle());

    graphicsContext->DrawFullScreenTriangle();

    graphicsContext->InsertPixEndEvent();
}


void DeferredRenderer::Render()
{
	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
    Camera *mainCamera = mActiveScene->GetMainCamera();

    //set up the camera buffer
    D3DXMATRIXA16 lightProj;
    D3DXMATRIXA16 lightView;
    D3DXMATRIX lightViewProj;
    D3DXMATRIXA16 centerTransform;
    Vector3 eye = mActiveScene->GetSunLight()->GetDirection() * -1.0f;
    Vector3 at(0.0f, 0.0f, 0.0f);
    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 frustumMin;
    Vector3 frustumMax;

    //to align to frustum
    up = mainCamera->ComputeInverseRight();
    D3DXMatrixLookAtLH(&lightView, &eye.AsD3DVector3(), &at.AsD3DVector3(), &up.AsD3DVector3());

    D3DXMATRIX cameraView = mainCamera->GetViewMatrix();
    D3DXMATRIX cameraViewInv;
    D3DXMatrixInverse(&cameraViewInv, NULL, &cameraView);

    MatrixHelper::CalculateFrustumExtentsD3DX(cameraViewInv, mainCamera->GetReverseProjectionMatrix(), mainCamera->GetScreenSettings().Near,
        mainCamera->GetScreenSettings().Far, lightView, frustumMin, frustumMax);

    Vector3 center = (frustumMin + frustumMax) * 0.5f;
    Vector3 dimensions = frustumMax - frustumMin;
        
    D3DXMatrixOrthoLH(&lightProj, dimensions.X, dimensions.Y, 0.0f, dimensions.Z);
    D3DXMatrixTranslation(&centerTransform, -center.X, -center.Y, -frustumMin.Z);
    lightView *= centerTransform;
    lightViewProj = lightView * lightProj;

    CameraBuffer cameraBuffer;
    cameraBuffer.viewMatrix = cameraView;
    cameraBuffer.viewInvMatrix = cameraViewInv;
    cameraBuffer.projectionMatrix = mainCamera->GetReverseProjectionMatrix();
    cameraBuffer.viewToLightProjMatrix = cameraBuffer.viewInvMatrix * lightViewProj;

    D3DXMatrixTranspose(&cameraBuffer.viewMatrix, &cameraBuffer.viewMatrix);
    D3DXMatrixTranspose(&cameraBuffer.viewInvMatrix, &cameraBuffer.viewInvMatrix);
    D3DXMatrixTranspose(&cameraBuffer.projectionMatrix, &cameraBuffer.projectionMatrix);
    D3DXMatrixTranspose(&cameraBuffer.viewToLightProjMatrix, &cameraBuffer.viewToLightProjMatrix);

    mCameraConstantBuffer[direct3DManager->GetFrameIndex()]->SetConstantBufferData(&cameraBuffer, sizeof(CameraBuffer));
    
    RenderRayTracing();
	ClearFrameBuffers();
	RenderGBuffer();
    RenderSky();
    RenderShadows(lightView, lightProj);
    ProcessRayShadows();
    RenderLightingMain(cameraBuffer.viewMatrix, cameraBuffer.projectionMatrix, cameraBuffer.viewToLightProjMatrix, cameraBuffer.viewInvMatrix);
    

    if (mRayTraceManager && mShowRayTrace)
    {
        mPostProcessManager->RenderPostProcessing(mRayShadowBlurTarget, 0.0167f);
    }
    else
    {
        mPostProcessManager->RenderPostProcessing(mHDRTarget, 0.0167f); //TDA FIX
    }

    direct3DManager->GetContextManager()->GetQueueManager()->GetGraphicsQueue()->WaitForFenceCPUBlocking(mPreviousFrameFence);

    direct3DManager->GetContextManager()->GetGraphicsContext(1)->Flush(direct3DManager->GetContextManager()->GetQueueManager());

    if (mRayTraceManager)
    {
        direct3DManager->GetContextManager()->GetRayTraceContext()->Flush(direct3DManager->GetContextManager()->GetQueueManager());
    }

    mPreviousFrameFence = direct3DManager->GetContextManager()->GetGraphicsContext()->Flush(direct3DManager->GetContextManager()->GetQueueManager());
}