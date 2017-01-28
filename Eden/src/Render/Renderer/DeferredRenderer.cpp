#include "Render/Renderer/DeferredRenderer.h"

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
	mShader = mGraphicsManager->GetShaderManager()->GetShaderTechnique("SimpleColor");
	mSampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_ANISO);
	mTexture = mGraphicsManager->GetTextureManager()->GetTexture("MageDiffuseFire");
	
	mMatrixConstantBuffer = contextManager->CreateConstantBuffer(sizeof(MatrixBufferTest));

	mMatrixBufferStart = mGBufferCBVDescHeap->GetHeapHandleBlock(1);
	mTextureStart = mGBufferCBVDescHeap->GetHeapHandleBlock(1);
	mSamplerStart = mGBufferSamplerDescHeap->GetHeapHandleBlock(1);

	direct3DManager->GetDevice()->CopyDescriptorsSimple(1, mMatrixBufferStart.GetCPUHandle(), mMatrixConstantBuffer->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	direct3DManager->GetDevice()->CopyDescriptorsSimple(1, mTextureStart.GetCPUHandle(), mTexture->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	direct3DManager->GetDevice()->CopyDescriptorsSimple(1, mSamplerStart.GetCPUHandle(), mSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	
	direct3DManager->WaitForGPU();
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

	heapManager->FreeSRVDescriptorHeapHandle(mMatrixConstantBuffer->GetConstantBufferViewHandle());
	delete mMatrixConstantBuffer;
	
	delete mGBufferCBVDescHeap;
	delete mGBufferSamplerDescHeap;
}

void DeferredRenderer::Render()
{
	{
		D3DXMATRIX modelMatrix, positionMatrix, rotationMatrix, scalarMatrix;
		D3DXMatrixIdentity(&positionMatrix);
		D3DXMatrixIdentity(&rotationMatrix);
		D3DXMatrixIdentity(&scalarMatrix);
		D3DXMatrixIdentity(&modelMatrix);
		D3DXMatrixTranslation(&positionMatrix, 0, -5, 20);
		D3DXMatrixRotationYawPitchRoll(&rotationMatrix, MathHelper::Radian() * 180.0f, 0, 0);//Rotation.Y, Rotation.X, Rotation.Z);
		D3DXMatrixScaling(&scalarMatrix, 1, 1, 1);
		D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &scalarMatrix);
		D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &rotationMatrix);
		D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &positionMatrix);

		MatrixBufferTest camBuff;
		camBuff.worldMatrix = modelMatrix;
		camBuff.viewMatrix = mActiveScene->GetMainCamera()->GetViewMatrix();
		camBuff.projectionMatrix = mActiveScene->GetMainCamera()->GetReverseProjectionMatrix();

		MatrixBufferTest camBuff2;
		D3DXMatrixTranspose(&camBuff2.worldMatrix, &camBuff.worldMatrix);
		D3DXMatrixTranspose(&camBuff2.viewMatrix, &camBuff.viewMatrix);
		D3DXMatrixTranspose(&camBuff2.projectionMatrix, &camBuff.projectionMatrix);

		mMatrixConstantBuffer->SetConstantBufferData(&camBuff2, sizeof(MatrixBufferTest));
	}


	Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
	GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();

	Vector2 screenSize = direct3DManager->GetScreenSize();
	graphicsContext->SetViewport(direct3DManager->GetScreenViewport());
	graphicsContext->SetScissorRect(0, 0, (uint32)screenSize.X, (uint32)screenSize.Y);

	BackBufferTarget *backBuffer = direct3DManager->GetBackBufferTarget();
	graphicsContext->TransitionResource((*backBuffer), D3D12_RESOURCE_STATE_RENDER_TARGET, true);

	static float blueSub = 0.0f;
	if (blueSub < 0.9f)
	{
	//	blueSub += 0.0001f;
	}

	// Record drawing commands.
	float color[4] = {0.392156899f, 0.584313750f, 0.929411829f - blueSub, 1.000000000f};
	graphicsContext->ClearRenderTarget(direct3DManager->GetRenderTargetView(), color);
	graphicsContext->ClearDepthStencilTarget(mGBufferDepth->GetDepthStencilViewHandle().GetCPUHandle(), 0.0f, 0);

	{
		graphicsContext->SetRenderTarget(backBuffer->GetRenderTargetViewHandle().GetCPUHandle(), mGBufferDepth->GetDepthStencilViewHandle().GetCPUHandle());
		
		ShaderPipelinePermutation permutation(Render_Standard, Target_Standard_BackBuffer);
		ShaderPSO *shaderPSO = mShader->GetShader(permutation);
		graphicsContext->SetPipelineState(shaderPSO);
		graphicsContext->SetRootSignature(shaderPSO->GetRootSignature());

		graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mGBufferCBVDescHeap->GetHeap()); //combine these
		graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, mGBufferSamplerDescHeap->GetHeap());

		graphicsContext->SetDescriptorTable(0, mTextureStart.GetGPUHandle());
		graphicsContext->SetDescriptorTable(1, mSamplerStart.GetGPUHandle());
		graphicsContext->SetDescriptorTable(2, mMatrixBufferStart.GetGPUHandle());

		graphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		graphicsContext->SetVertexBuffer(0, mMesh->GetVertexBuffer());
		graphicsContext->SetIndexBuffer(mMesh->GetIndexBuffer());
		graphicsContext->Draw(mMesh->GetVertexCount());
	}

	graphicsContext->TransitionResource((*backBuffer), D3D12_RESOURCE_STATE_PRESENT, true);

	graphicsContext->Flush(direct3DManager->GetContextManager()->GetQueueManager(), true);
}