#include "Render/Renderer/DeferredRenderer.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"

DeferredRenderer::DeferredRenderer(GraphicsManager *graphicsManager)
{
	mGraphicsManager = graphicsManager;
	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	Direct3DContextManager *contextManager = direct3DManager->GetContextManager();
	contextManager->GetGraphicsContext()->Flush(contextManager->GetQueueManager(), true);
	
	CreateTargets(direct3DManager->GetScreenSize());

	//need to be able to make these bigger
	mGBufferCBVDescHeap = new RenderPassDescriptorHeap(direct3DManager->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024, true);
	mGBufferSamplerDescHeap = new RenderPassDescriptorHeap(direct3DManager->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 16, true);
	
	mCameraConstantBuffer = contextManager->CreateConstantBuffer(sizeof(CameraBuffer));

	{
		Material *newMaterial = new Material(contextManager->CreateConstantBuffer(sizeof(MaterialConstants)));
		newMaterial->SetTexture(MaterialTextureType_Diffuse, mGraphicsManager->GetTextureManager()->GetTexture("MageDiffuseFire"));
		newMaterial->SetTexture(MaterialTextureType_Normal, mGraphicsManager->GetTextureManager()->GetTexture("MageNormal"));
		newMaterial->SetTexture(MaterialTextureType_Roughmetal, mGraphicsManager->GetTextureManager()->GetTexture("MageRoughMetal"));
		newMaterial->GetMaterialBuffer()->SetUsesNormalMap(true);
		newMaterial->GetMaterialBuffer()->SetUsesRoughmetalMap(true);
		mSceneEntity = new SceneEntity(mGraphicsManager->GetMeshManager()->GetMesh("MageBiNormals"), newMaterial);
		mSceneEntity->SetPosition(Vector3(0, -5.0f, 20.0f));
		mSceneEntity->SetRotation(Vector3(0, MathHelper::Radian() * 180.0f, 0));
	}
	{
		Material *newMaterial = new Material(contextManager->CreateConstantBuffer(sizeof(MaterialConstants)));
		newMaterial->SetTexture(MaterialTextureType_Diffuse, mGraphicsManager->GetTextureManager()->GetTexture("MageDiffuseFire"));
		newMaterial->SetTexture(MaterialTextureType_Normal, mGraphicsManager->GetTextureManager()->GetTexture("MageNormal"));
		newMaterial->SetTexture(MaterialTextureType_Roughmetal, mGraphicsManager->GetTextureManager()->GetTexture("MageRoughMetal"));
		newMaterial->GetMaterialBuffer()->SetUsesNormalMap(true);
		newMaterial->GetMaterialBuffer()->SetUsesRoughmetalMap(true);
		mSceneEntity2 = new SceneEntity(mGraphicsManager->GetMeshManager()->GetMesh("MageBiNormals"), newMaterial);
		mSceneEntity2->SetPosition(Vector3(10, -5.0f, 20.0f));
		mSceneEntity2->SetRotation(Vector3(0, MathHelper::Radian() * 180.0f, 0));
	}
	{
		Material *newMaterial = new Material(contextManager->CreateConstantBuffer(sizeof(MaterialConstants)));
		newMaterial->SetTexture(MaterialTextureType_Diffuse, mGraphicsManager->GetTextureManager()->GetTexture("MageDiffuseFire"));
		newMaterial->SetTexture(MaterialTextureType_Normal, mGraphicsManager->GetTextureManager()->GetTexture("MageNormal"));
		newMaterial->SetTexture(MaterialTextureType_Roughmetal, mGraphicsManager->GetTextureManager()->GetTexture("MageRoughMetal"));
		newMaterial->GetMaterialBuffer()->SetUsesNormalMap(true);
		newMaterial->GetMaterialBuffer()->SetUsesRoughmetalMap(true);
		mSceneEntity3 = new SceneEntity(mGraphicsManager->GetMeshManager()->GetMesh("MageBiNormals"), newMaterial);
		mSceneEntity3->SetPosition(Vector3(-10, -5.0f, 20.0f));
		mSceneEntity3->SetRotation(Vector3(0, MathHelper::Radian() * 180.0f, 0));
	}
}

DeferredRenderer::~DeferredRenderer()
{
	FreeTargets();

	Direct3DContextManager *contextManager = mGraphicsManager->GetDirect3DManager()->GetContextManager();
	contextManager->FreeConstantBuffer(mCameraConstantBuffer);
	mCameraConstantBuffer = NULL;
	
	{
		Material *material = mSceneEntity->GetMaterial();
		mGraphicsManager->GetDirect3DManager()->GetContextManager()->FreeConstantBuffer(material->GetConstantBuffer());
		delete material;
		delete mSceneEntity;
	}
	{
		Material *material = mSceneEntity2->GetMaterial();
		mGraphicsManager->GetDirect3DManager()->GetContextManager()->FreeConstantBuffer(material->GetConstantBuffer());
		delete material;
		delete mSceneEntity2;
	}
	{
		Material *material = mSceneEntity3->GetMaterial();
		mGraphicsManager->GetDirect3DManager()->GetContextManager()->FreeConstantBuffer(material->GetConstantBuffer());
		delete material;
		delete mSceneEntity3;
	}

	delete mGBufferCBVDescHeap;
	delete mGBufferSamplerDescHeap;
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
}

void DeferredRenderer::OnScreenChanged(Vector2 screenSize)
{
	FreeTargets();
	CreateTargets(screenSize);
}

void DeferredRenderer::ClearGBuffer()
{
	GraphicsContext *graphicsContext = mGraphicsManager->GetDirect3DManager()->GetContextManager()->GetGraphicsContext();
	float blackColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	graphicsContext->TransitionResource((*mGBufferTargets[0]), D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	graphicsContext->TransitionResource((*mGBufferTargets[1]), D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	graphicsContext->TransitionResource((*mGBufferTargets[2]), D3D12_RESOURCE_STATE_RENDER_TARGET, true);

	graphicsContext->ClearRenderTarget(mGBufferTargets[0]->GetRenderTargetViewHandle().GetCPUHandle(), blackColor);
	graphicsContext->ClearRenderTarget(mGBufferTargets[1]->GetRenderTargetViewHandle().GetCPUHandle(), blackColor);
	graphicsContext->ClearRenderTarget(mGBufferTargets[2]->GetRenderTargetViewHandle().GetCPUHandle(), blackColor);
	graphicsContext->ClearDepthStencilTarget(mGBufferDepth->GetDepthStencilViewHandle().GetCPUHandle(), 0.0f, 0);
}

void DeferredRenderer::RenderGBuffer()
{
	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
	Vector2 screenSize = direct3DManager->GetScreenSize();

	graphicsContext->SetViewport(direct3DManager->GetScreenViewport());
	graphicsContext->SetScissorRect(0, 0, (uint32)screenSize.X, (uint32)screenSize.Y);

	// set up the default samplers
	DescriptorHeapHandle gBufferSamplersHandle = mGBufferSamplerDescHeap->GetHeapHandleBlock(3);
	D3D12_CPU_DESCRIPTOR_HANDLE gBufferSamplersHandleOffset = gBufferSamplersHandle.GetCPUHandle();
	Sampler *defaultAnisoSampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_ANISO);

	for (uint32 i = 0; i < GBufferTextureInputCount; i++)
	{
		direct3DManager->GetDevice()->CopyDescriptorsSimple(1, gBufferSamplersHandleOffset, defaultAnisoSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		gBufferSamplersHandleOffset.ptr += mGBufferSamplerDescHeap->GetDescriptorSize();
	}

	//set up the camera buffer
	CameraBuffer cameraBuffer;
	cameraBuffer.viewMatrix = mActiveScene->GetMainCamera()->GetViewMatrix();
	cameraBuffer.projectionMatrix = mActiveScene->GetMainCamera()->GetReverseProjectionMatrix();
	D3DXMatrixTranspose(&cameraBuffer.viewMatrix, &cameraBuffer.viewMatrix);
	D3DXMatrixTranspose(&cameraBuffer.projectionMatrix, &cameraBuffer.projectionMatrix);

	mCameraConstantBuffer->SetConstantBufferData(&cameraBuffer, sizeof(CameraBuffer));

	DescriptorHeapHandle perFrameCameraHandle = mGBufferCBVDescHeap->GetHeapHandleBlock(1);
	direct3DManager->GetDevice()->CopyDescriptorsSimple(1, perFrameCameraHandle.GetCPUHandle(), mCameraConstantBuffer->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//set up the render targets
	D3D12_CPU_DESCRIPTOR_HANDLE gbufferRtvHandles[3] = {
		mGBufferTargets[0]->GetRenderTargetViewHandle().GetCPUHandle(),
		mGBufferTargets[1]->GetRenderTargetViewHandle().GetCPUHandle(),
		mGBufferTargets[2]->GetRenderTargetViewHandle().GetCPUHandle()
	};
	graphicsContext->SetRenderTargets(3, gbufferRtvHandles, mGBufferDepth->GetDepthStencilViewHandle().GetCPUHandle());

	//shader setup
	ShaderPipelinePermutation permutation(Render_Standard, Target_GBuffer);
	ShaderPSO *shaderPSO = mGraphicsManager->GetShaderManager()->GetShader("GBufferLit", permutation);
	graphicsContext->SetPipelineState(shaderPSO);
	graphicsContext->SetRootSignature(shaderPSO->GetRootSignature());

	graphicsContext->SetDescriptorTable(2, perFrameCameraHandle.GetGPUHandle());
	graphicsContext->SetDescriptorTable(3, gBufferSamplersHandle.GetGPUHandle());

	DynamicArray<MaterialTextureType> textureTypes;
	textureTypes.Add(MaterialTextureType_Diffuse);
	textureTypes.Add(MaterialTextureType_Normal);
	textureTypes.Add(MaterialTextureType_Roughmetal);
	RenderPassContext renderPassContext(graphicsContext, mGBufferCBVDescHeap, textureTypes);
	mSceneEntity->Render(&renderPassContext);
	mSceneEntity2->Render(&renderPassContext);
	mSceneEntity3->Render(&renderPassContext);
}

void DeferredRenderer::CopyToBackBuffer(RenderTarget *renderTargetToCopy)
{
	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
	BackBufferTarget *backBuffer = direct3DManager->GetBackBufferTarget();

	DescriptorHeapHandle copyTextureHandle = mGBufferCBVDescHeap->GetHeapHandleBlock(1);
	DescriptorHeapHandle copySamplerHandle = mGBufferSamplerDescHeap->GetHeapHandleBlock(1);

	direct3DManager->GetDevice()->CopyDescriptorsSimple(1, copyTextureHandle.GetCPUHandle(), renderTargetToCopy->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	direct3DManager->GetDevice()->CopyDescriptorsSimple(1, copySamplerHandle.GetCPUHandle(), mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_POINT)->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	graphicsContext->TransitionResource((*backBuffer), D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	graphicsContext->TransitionResource((*renderTargetToCopy), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);

	graphicsContext->SetRenderTarget(backBuffer->GetRenderTargetViewHandle().GetCPUHandle());

	ShaderPipelinePermutation bbPermutation(Render_Standard_NoDepth, Target_Standard_BackBuffer_NoDepth);
	ShaderPSO *copyShader = mGraphicsManager->GetShaderManager()->GetShader("SimpleCopy", bbPermutation);
	graphicsContext->SetPipelineState(copyShader);
	graphicsContext->SetRootSignature(copyShader->GetRootSignature());

	graphicsContext->SetDescriptorTable(0, copyTextureHandle.GetGPUHandle());
	graphicsContext->SetDescriptorTable(1, copySamplerHandle.GetGPUHandle());

	graphicsContext->DrawFullScreenTriangle();

	graphicsContext->TransitionResource((*backBuffer), D3D12_RESOURCE_STATE_PRESENT, true);
}

void DeferredRenderer::Render()
{
	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();

	//Reset and set heaps
	mGBufferCBVDescHeap->Reset();
	mGBufferSamplerDescHeap->Reset();
	graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mGBufferCBVDescHeap->GetHeap());
	graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, mGBufferSamplerDescHeap->GetHeap());

	ClearGBuffer();

	RenderGBuffer();

	CopyToBackBuffer(mGBufferTargets[0]);

	graphicsContext->Flush(direct3DManager->GetContextManager()->GetQueueManager(), true);
}