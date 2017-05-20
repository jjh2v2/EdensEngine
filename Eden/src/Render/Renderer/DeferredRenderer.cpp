#include "Render/Renderer/DeferredRenderer.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"

DeferredRenderer::DeferredRenderer(GraphicsManager *graphicsManager)
{
	mGraphicsManager = graphicsManager;
	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	direct3DManager->WaitForGPU();
	
	//need to be able to make these bigger
	mGBufferCBVDescHeap = new RenderPassDescriptorHeap(direct3DManager->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024, true);
	mGBufferSamplerDescHeap = new RenderPassDescriptorHeap(direct3DManager->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 16, true);

	Vector2 screenSize = direct3DManager->GetScreenSize();
	Direct3DContextManager *contextManager = direct3DManager->GetContextManager();
	RenderTarget *gbufferAlbedoTarget   = contextManager->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
	RenderTarget *gbufferNormalsTarget  = contextManager->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
	RenderTarget *gbufferMaterialTarget = contextManager->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
	mGBufferTargets.Add(gbufferAlbedoTarget);
	mGBufferTargets.Add(gbufferNormalsTarget);
	mGBufferTargets.Add(gbufferMaterialTarget);

	mGBufferDepth = contextManager->CreateDepthStencilTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_D32_FLOAT_S8X24_UINT, 1, 1, 0);

	mMesh = mGraphicsManager->GetMeshManager()->GetMesh("MageBiNormals");
	mTexture = mGraphicsManager->GetTextureManager()->GetTexture("MageDiffuseFire");
	
	mCameraConstantBuffer = contextManager->CreateConstantBuffer(sizeof(CameraBuffer));
	mMaterialConstantBuffer = contextManager->CreateConstantBuffer(sizeof(MaterialConstants));
}

DeferredRenderer::~DeferredRenderer()
{
	Direct3DHeapManager *heapManager = mGraphicsManager->GetDirect3DManager()->GetContextManager()->GetHeapManager();

	for (uint32 targetIndex = 0; targetIndex < mGBufferTargets.CurrentSize(); targetIndex++)
	{
		RenderTarget *currentTarget = mGBufferTargets[targetIndex];
		heapManager->FreeRTVDescriptorHeapHandle(currentTarget->GetRenderTargetViewHandle());
		heapManager->FreeSRVDescriptorHeapHandle(currentTarget->GetShaderResourceViewHandle());
		RenderTarget::UAVHandle uavHandle = currentTarget->GetUnorderedAccessViewHandle();

		if (uavHandle.HasUAV)
		{
			heapManager->FreeSRVDescriptorHeapHandle(uavHandle.Handle);
		}

		uint16 targetArraySize = currentTarget->GetArraySize() - 1;
		for (uint16 rtvArrayIndex = 0; rtvArrayIndex < targetArraySize; rtvArrayIndex++)
		{
			heapManager->FreeRTVDescriptorHeapHandle(currentTarget->GetRenderTargetViewHandle(rtvArrayIndex));
		}

		delete currentTarget;
	}

	if(mGBufferDepth)
	{
		heapManager->FreeDSVDescriptorHeapHandle(mGBufferDepth->GetDepthStencilViewHandle());
		heapManager->FreeDSVDescriptorHeapHandle(mGBufferDepth->GetReadOnlyDepthStencilViewHandle());
		heapManager->FreeSRVDescriptorHeapHandle(mGBufferDepth->GetShaderResourceViewHandle());

		uint16 targetArraySize = mGBufferDepth->GetArraySize() - 1;
		for (uint16 dsvArrayIndex = 0; dsvArrayIndex < targetArraySize; dsvArrayIndex++)
		{
			heapManager->FreeDSVDescriptorHeapHandle(mGBufferDepth->GetDepthStencilViewHandle(dsvArrayIndex));
			heapManager->FreeDSVDescriptorHeapHandle(mGBufferDepth->GetReadOnlyDepthStencilViewHandle(dsvArrayIndex));
		}

		delete mGBufferDepth;
		mGBufferDepth = NULL;
	}

	heapManager->FreeSRVDescriptorHeapHandle(mCameraConstantBuffer->GetConstantBufferViewHandle());
	delete mCameraConstantBuffer;

	heapManager->FreeSRVDescriptorHeapHandle(mMaterialConstantBuffer->GetConstantBufferViewHandle());
	delete mMaterialConstantBuffer;
	
	delete mGBufferCBVDescHeap;
	delete mGBufferSamplerDescHeap;
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
	ShaderPSO *shaderPSO = mGraphicsManager->GetShaderManager()->GetShaderTechnique("GBufferLit")->GetShader(permutation);
	graphicsContext->SetPipelineState(shaderPSO);
	graphicsContext->SetRootSignature(shaderPSO->GetRootSignature());

	graphicsContext->SetDescriptorTable(2, perFrameCameraHandle.GetGPUHandle());
	graphicsContext->SetDescriptorTable(3, gBufferSamplersHandle.GetGPUHandle());
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
	ShaderPSO *copyShader = mGraphicsManager->GetShaderManager()->GetShaderTechnique("SimpleCopy")->GetShader(bbPermutation);
	graphicsContext->SetPipelineState(copyShader);
	graphicsContext->SetRootSignature(copyShader->GetRootSignature());

	graphicsContext->SetDescriptorTable(0, copyTextureHandle.GetGPUHandle());
	graphicsContext->SetDescriptorTable(1, copySamplerHandle.GetGPUHandle());

	graphicsContext->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	graphicsContext->SetVertexBuffer(0, NULL);
	graphicsContext->SetIndexBuffer(NULL);
	graphicsContext->Draw(3);

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

	{
		mMaterialBufferStart = mGBufferCBVDescHeap->GetHeapHandleBlock(1);
		mTextureStart = mGBufferCBVDescHeap->GetHeapHandleBlock(3);

		D3D12_CPU_DESCRIPTOR_HANDLE textureOffsetHandle = mTextureStart.GetCPUHandle();

		direct3DManager->GetDevice()->CopyDescriptorsSimple(1, mMaterialBufferStart.GetCPUHandle(), mMaterialConstantBuffer->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		direct3DManager->GetDevice()->CopyDescriptorsSimple(1, textureOffsetHandle, mTexture->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		textureOffsetHandle.ptr += mGBufferCBVDescHeap->GetDescriptorSize();
		direct3DManager->GetDevice()->CopyDescriptorsSimple(1, textureOffsetHandle, mGraphicsManager->GetTextureManager()->GetTexture("MageNormal")->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		textureOffsetHandle.ptr += mGBufferCBVDescHeap->GetDescriptorSize();
		direct3DManager->GetDevice()->CopyDescriptorsSimple(1, textureOffsetHandle, mGraphicsManager->GetTextureManager()->GetTexture("MageRoughMetal")->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	{
		D3DXMATRIX modelMatrix, positionMatrix, rotationMatrix, scalarMatrix;
		D3DXMatrixIdentity(&positionMatrix);
		D3DXMatrixIdentity(&rotationMatrix);
		D3DXMatrixIdentity(&scalarMatrix);
		D3DXMatrixIdentity(&modelMatrix);
		D3DXMatrixTranslation(&positionMatrix, 0, -5, 20);
		D3DXMatrixRotationYawPitchRoll(&rotationMatrix, MathHelper::Radian() * 180.0f, 0, 0);
		D3DXMatrixScaling(&scalarMatrix, 1, 1, 1);
		D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &scalarMatrix);
		D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &rotationMatrix);
		D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &positionMatrix);
		D3DXMatrixTranspose(&modelMatrix, &modelMatrix);

		MaterialConstants matConstants;
		matConstants.diffuseColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
		matConstants.materialIntensity = 1.0f;
		matConstants.metalness = 0.0f;
		matConstants.roughness = 1.0f;
		matConstants.tiling = Vector2(1.0f, 1.0f);
		matConstants.usesNormalMap = 1;
		matConstants.usesRoughMetalMap = 1;
		matConstants.worldMatrix = modelMatrix;

		mMaterialConstantBuffer->SetConstantBufferData(&matConstants, sizeof(MaterialConstants));
	}

	

	ClearGBuffer();

	RenderGBuffer();

	{
		graphicsContext->SetDescriptorTable(0, mTextureStart.GetGPUHandle());
		graphicsContext->SetDescriptorTable(1, mMaterialBufferStart.GetGPUHandle());
		
		graphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		graphicsContext->SetVertexBuffer(0, mMesh->GetVertexBuffer());
		graphicsContext->SetIndexBuffer(mMesh->GetIndexBuffer());
		graphicsContext->Draw(mMesh->GetVertexCount());
	}

	CopyToBackBuffer(mGBufferTargets[1]);

	graphicsContext->Flush(direct3DManager->GetContextManager()->GetQueueManager(), true);
}