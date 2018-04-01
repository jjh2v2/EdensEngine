#include "Render/Shadow/SDSMShadowManager.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"
#include "Render/Shader/Definitions/StructuredBufferDefinitions.h"
#include "Camera/Camera.h"
#include "Entity/SceneEntity.h"

SDSMShadowManager::SDSMShadowManager(GraphicsManager *graphicsManager)
{
    mGraphicsManager = graphicsManager;

    ShaderManager *shaderManager = mGraphicsManager->GetShaderManager();
    ShaderPipelinePermutation computeShaderEmptyPermutation;

    mClearShadowPartitionsShader = shaderManager->GetShader("ClearShadowPartitions", computeShaderEmptyPermutation);
    mCalculateDepthBufferBoundsShader = shaderManager->GetShader("CalculateDepthBufferBounds", computeShaderEmptyPermutation);
    mCalculateLogPartitionsFromDepthBoundsShader = shaderManager->GetShader("CalculateLogPartitionsFromDepthBounds", computeShaderEmptyPermutation);
    mClearShadowPartitionBoundsShader = shaderManager->GetShader("ClearShadowPartitionBounds", computeShaderEmptyPermutation);
    mCalculatePartitionBoundsShader = shaderManager->GetShader("CalculatePartitionBounds", computeShaderEmptyPermutation);
    mFinalizePartitionsShader = shaderManager->GetShader("FinalizePartitions", computeShaderEmptyPermutation);

    Direct3DContextManager *contextManager = mGraphicsManager->GetDirect3DManager()->GetContextManager();

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mSDSMBuffers[i] = contextManager->CreateConstantBuffer(sizeof(SDSMBuffer));
    }

    mShadowPartitionBuffer = contextManager->CreateStructuredBuffer(sizeof(SDSMPartition), mShadowPreferences.PartitionCount, GPU_READ_WRITE, false);
    mShadowPartitionBoundsBuffer = contextManager->CreateStructuredBuffer(sizeof(SDSMBounds), mShadowPreferences.PartitionCount, GPU_READ_WRITE, false);

    mShadowDepthTarget = contextManager->CreateDepthStencilTarget(mShadowPreferences.ShadowTextureSize, mShadowPreferences.ShadowTextureSize, DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
        1, mShadowPreferences.ShadowAntiAliasingSamples, 0);

    for (uint32 i = 0; i < SDSM_SHADOW_PARTITION_COUNT; i++)
    {
        mShadowEVSMTextures[i] = contextManager->CreateRenderTarget(mShadowPreferences.ShadowTextureSize, mShadowPreferences.ShadowTextureSize, 
            DXGI_FORMAT_R32G32B32A32_FLOAT, false, 1, 1, 0, 0);
    }

    mShadowEVSMBlurTexture = contextManager->CreateRenderTarget(mShadowPreferences.ShadowTextureSize, mShadowPreferences.ShadowTextureSize,
        DXGI_FORMAT_R32G32B32A32_FLOAT, false, 1, 1, 0);


    mShadowMapViewport.Width = (float)mShadowPreferences.ShadowTextureSize;
    mShadowMapViewport.Height = (float)mShadowPreferences.ShadowTextureSize;
    mShadowMapViewport.MinDepth = 0.0f;
    mShadowMapViewport.MaxDepth = 1.0f;
    mShadowMapViewport.TopLeftX = 0.0f;
    mShadowMapViewport.TopLeftY = 0.0f;

    mPreviousShadowPassFence = 0;
}

SDSMShadowManager::~SDSMShadowManager()
{
    Direct3DContextManager *contextManager = mGraphicsManager->GetDirect3DManager()->GetContextManager();

    contextManager->FreeRenderTarget(mShadowEVSMBlurTexture);

    for (uint32 i = 0; i < SDSM_SHADOW_PARTITION_COUNT; i++)
    {
        contextManager->FreeRenderTarget(mShadowEVSMTextures[i]);
    }

    contextManager->FreeDepthStencilTarget(mShadowDepthTarget);
    contextManager->FreeStructuredBuffer(mShadowPartitionBoundsBuffer);
    contextManager->FreeStructuredBuffer(mShadowPartitionBuffer);

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        contextManager->FreeConstantBuffer(mSDSMBuffers[i]);
        mSDSMBuffers[i] = NULL;
    }

    mClearShadowPartitionsShader = NULL;
    mCalculateDepthBufferBoundsShader = NULL;
    mCalculateLogPartitionsFromDepthBoundsShader = NULL;
    mClearShadowPartitionBoundsShader = NULL;
    mCalculatePartitionBoundsShader = NULL;
    mFinalizePartitionsShader = NULL;

    mGraphicsManager = NULL;
}

void SDSMShadowManager::ComputeShadowPartitions(Camera *camera, D3DXMATRIX &lightViewMatrix, D3DXMATRIX &lightProjectionMatrix, DepthStencilTarget *depthStencil)
{
    Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();

    Camera::CameraScreenSettings cameraSettings = camera->GetScreenSettings();
    const float maxFloat = FLT_MAX;
    Vector3 maxPartitionScale(maxFloat, maxFloat, maxFloat);

    mBlurSizeInLightSpace = Vector2(0.0f, 0.0f);

    if (mShadowPreferences.UseSoftShadows)
    {
        mBlurSizeInLightSpace.X = mShadowPreferences.SofteningAmount * 0.5f * lightProjectionMatrix._11;
        mBlurSizeInLightSpace.Y = mShadowPreferences.SofteningAmount * 0.5f * lightProjectionMatrix._22;

        float maxBlurLightSpace = mShadowPreferences.MaxSofteningFilter / ((float)mShadowPreferences.ShadowTextureSize);
        maxPartitionScale.X = maxBlurLightSpace / mBlurSizeInLightSpace.X;
        maxPartitionScale.Y = maxBlurLightSpace / mBlurSizeInLightSpace.Y;
    }

    Vector3 partitionBorderLightSpace(mBlurSizeInLightSpace.X, mBlurSizeInLightSpace.Y, 1.0f);
    partitionBorderLightSpace.Z *= lightProjectionMatrix._33;

    D3DXMATRIX cameraViewInvMatrix;
    D3DXMATRIX cameraViewMatrix = camera->GetViewMatrix();
    D3DXMatrixInverse(&cameraViewInvMatrix, NULL, &cameraViewMatrix);
    D3DXMATRIX lightViewProjMatrix = lightViewMatrix * lightProjectionMatrix;

    mCurrentSDSMBuffer.cameraProjMatrix = camera->GetReverseProjectionMatrix();
    mCurrentSDSMBuffer.cameraViewToLightProjMatrix = cameraViewInvMatrix * lightViewProjMatrix;
    mCurrentSDSMBuffer.lightSpaceBorder = Vector4(partitionBorderLightSpace.X, partitionBorderLightSpace.Y, partitionBorderLightSpace.Z, 0.0f);
    mCurrentSDSMBuffer.maxScale = Vector4(maxPartitionScale.X, maxPartitionScale.Y, maxPartitionScale.Z, 0.0f);
    mCurrentSDSMBuffer.bufferDimensions = direct3DManager->GetScreenSize();
    mCurrentSDSMBuffer.cameraNearFar = Vector2(cameraSettings.Far, cameraSettings.Near); //purposefully flipped
    mCurrentSDSMBuffer.dilationFactor = 0.01f;
    mCurrentSDSMBuffer.reduceTileDim = 128;

    mSDSMBuffers[direct3DManager->GetFrameIndex()]->SetConstantBufferData(&mCurrentSDSMBuffer, sizeof(SDSMBuffer));

    Direct3DQueueManager *queueManager = direct3DManager->GetContextManager()->GetQueueManager();
    Direct3DQueue *computeQueue = queueManager->GetComputeQueue();

    computeQueue->WaitForFenceCPUBlocking(mPreviousShadowPassFence); //TDA: To remove this, will need to double-buffer, but we probably won't stall on this

    uint32 shadowNumSRVDescs = 1;
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();
    RenderPassDescriptorHeap *shadowSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_Shadows, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), shadowNumSRVDescs);

    ComputeContext *computeContext = direct3DManager->GetContextManager()->GetComputeContext();
    computeContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, shadowSRVDescHeap->GetHeap());

    ////// Clear partitions

    DescriptorHeapHandle shadowPartitionUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionUAVHandle.GetCPUHandle(), mShadowPartitionBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    computeContext->SetPipelineState(mClearShadowPartitionsShader);
    computeContext->SetRootSignature(mClearShadowPartitionsShader->GetRootSignature());
    computeContext->SetDescriptorTable(0, shadowPartitionUAVHandle.GetGPUHandle());

    computeContext->Dispatch(1, 1, 1);

    ////// Clear Partition Bounds

    DescriptorHeapHandle shadowPartitionBoundsUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionBoundsUAVHandle.GetCPUHandle(), mShadowPartitionBoundsBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    computeContext->SetPipelineState(mClearShadowPartitionBoundsShader);
    computeContext->SetRootSignature(mClearShadowPartitionBoundsShader->GetRootSignature());
    computeContext->SetDescriptorTable(0, shadowPartitionBoundsUAVHandle.GetGPUHandle());

    computeContext->Dispatch(1, 1, 1);

    ////// Find Depth Bounds

    shadowPartitionUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionUAVHandle.GetCPUHandle(), mShadowPartitionBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    DescriptorHeapHandle shadowPartitionSRVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionSRVHandle.GetCPUHandle(), depthStencil->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    DescriptorHeapHandle shadowPartitionCBVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionCBVHandle.GetCPUHandle(), mSDSMBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    computeContext->SetPipelineState(mCalculateDepthBufferBoundsShader);
    computeContext->SetRootSignature(mCalculateDepthBufferBoundsShader->GetRootSignature());
    computeContext->SetDescriptorTable(0, shadowPartitionUAVHandle.GetGPUHandle());
    computeContext->SetDescriptorTable(1, shadowPartitionSRVHandle.GetGPUHandle());
    computeContext->SetDescriptorTable(2, shadowPartitionCBVHandle.GetGPUHandle());

    uint32 dispatchWidth  = ((uint32)direct3DManager->GetScreenSize().X + mCurrentSDSMBuffer.reduceTileDim - 1) / mCurrentSDSMBuffer.reduceTileDim;
    uint32 dispatchHeight = ((uint32)direct3DManager->GetScreenSize().Y + mCurrentSDSMBuffer.reduceTileDim - 1) / mCurrentSDSMBuffer.reduceTileDim;

    computeContext->Dispatch(dispatchWidth, dispatchHeight, 1);

    ////// Get Partitions From Depth Bounds

    shadowPartitionUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionUAVHandle.GetCPUHandle(), mShadowPartitionBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionCBVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionCBVHandle.GetCPUHandle(), mSDSMBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    computeContext->SetPipelineState(mCalculateLogPartitionsFromDepthBoundsShader);
    computeContext->SetRootSignature(mCalculateLogPartitionsFromDepthBoundsShader->GetRootSignature());
    computeContext->SetDescriptorTable(0, shadowPartitionUAVHandle.GetGPUHandle());
    computeContext->SetDescriptorTable(1, shadowPartitionCBVHandle.GetGPUHandle());

    computeContext->Dispatch(1, 1, 1);

    ////// Get Partition Bounds Range From Partitions

    shadowPartitionBoundsUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionBoundsUAVHandle.GetCPUHandle(), mShadowPartitionBoundsBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionSRVHandle = shadowSRVDescHeap->GetHeapHandleBlock(2);
    D3D12_CPU_DESCRIPTOR_HANDLE currentTextureHandle = shadowPartitionSRVHandle.GetCPUHandle();

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentTextureHandle, depthStencil->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    currentTextureHandle.ptr += shadowSRVDescHeap->GetDescriptorSize();
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentTextureHandle, mShadowPartitionBuffer->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionCBVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionCBVHandle.GetCPUHandle(), mSDSMBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    computeContext->SetPipelineState(mCalculatePartitionBoundsShader);
    computeContext->SetRootSignature(mCalculatePartitionBoundsShader->GetRootSignature());
    computeContext->SetDescriptorTable(0, shadowPartitionBoundsUAVHandle.GetGPUHandle());
    computeContext->SetDescriptorTable(1, shadowPartitionSRVHandle.GetGPUHandle());
    computeContext->SetDescriptorTable(2, shadowPartitionCBVHandle.GetGPUHandle());

    computeContext->Dispatch(dispatchWidth, dispatchHeight, 1);

    ////// Finalize Partition Bounds

    shadowPartitionUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionUAVHandle.GetCPUHandle(), mShadowPartitionBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionSRVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentTextureHandle, mShadowPartitionBoundsBuffer->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionCBVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionCBVHandle.GetCPUHandle(), mSDSMBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    computeContext->SetPipelineState(mFinalizePartitionsShader);
    computeContext->SetRootSignature(mFinalizePartitionsShader->GetRootSignature());
    computeContext->SetDescriptorTable(0, shadowPartitionUAVHandle.GetGPUHandle());
    computeContext->SetDescriptorTable(1, shadowPartitionSRVHandle.GetGPUHandle());
    computeContext->SetDescriptorTable(2, shadowPartitionCBVHandle.GetGPUHandle());

    computeContext->Dispatch(1, 1, 1);
    mPreviousShadowPassFence = computeContext->Flush(queueManager);
}



void SDSMShadowManager::RenderShadowMapPartitions(const D3DXMATRIX &lightViewProjMatrix)
{
	// Generate raw depth map
    for (uint32 i = 0; i < SDSM_SHADOW_PARTITION_COUNT; i++)
    {
        RenderShadowDepth(i, lightViewProjMatrix);
    }

	// Convert single depth map to an EVSM in the proper array slice
	//ConvertToEVSM(direct3DManager, partitionSRV, partitionIndex);

	//if (mPreferences.UseSoftShadows) 
	//{
	//	BoxBlur(direct3DManager, partitionIndex, partitionSRV, scene);
	//}

	//direct3DManager->GetDeviceContext()->GenerateMips(mShadowEVSMTextures[partitionIndex]->GetShaderResourceView());
}

void SDSMShadowManager::RenderShadowDepth(uint32 partitionIndex, const D3DXMATRIX &lightViewProjMatrix, DynamicArray<SceneEntity*> &shadowEntities)
{
    Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();

    graphicsContext->ClearDepthStencilTarget(mShadowDepthTarget->GetDepthStencilViewHandle().GetCPUHandle(), 1.0f, 0);

    graphicsContext->SetViewport(mShadowMapViewport);
    graphicsContext->SetScissorRect(0, 0, mShadowPreferences.ShadowTextureSize, mShadowPreferences.ShadowTextureSize);
    graphicsContext->SetDepthStencilTarget(mShadowDepthTarget->GetDepthStencilViewHandle().GetCPUHandle());

    ShaderPipelinePermutation shaderPermutation(Render_ShadowMap, Target_Single_32_NoDepth, InputLayout_Standard);
    ShaderPSO *shaderPSO = mGraphicsManager->GetShaderManager()->GetShader("ShadowMap", shaderPermutation);
    graphicsContext->SetPipelineState(shaderPSO);
    graphicsContext->SetRootSignature(shaderPSO->GetRootSignature());

	//scene.RenderToShadowMap(direct3DManager->GetDeviceContext(), partitionSRV, lightViewProject, partitionIndex);

	//direct3DManager->ClearDeviceTargetsAndResources();*/
}

/*
void SDSMShadowManager::ConvertToEVSM(Direct3DManager *direct3DManager,	ID3D11ShaderResourceView* partitionSRV,	int partitionIndex)
{
	direct3DManager->SetupForFullScreenTriangle();
	direct3DManager->UseDefaultRasterState();
	direct3DManager->DisableAlphaBlending();

	direct3DManager->GetDeviceContext()->RSSetViewports(1, &mShadowViewport);

	ID3D11RenderTargetView *target = mShadowEVSMTextures[partitionIndex]->GetRenderTargetView();
	direct3DManager->GetDeviceContext()->OMSetRenderTargets(1, &target, 0);

	ShaderParams shaderParams;
	shaderParams.ShadowMap = mShadowDepthTexture->GetShaderResourceView();
	shaderParams.PartitionResourceView = partitionSRV;
	shaderParams.PartitionIndex = partitionIndex;

	mShadowMapToEVSMMaterial->SetShaderParameters(direct3DManager->GetDeviceContext(), shaderParams);
	mShadowMapToEVSMMaterial->Render(direct3DManager->GetDeviceContext());
	direct3DManager->GetDeviceContext()->Draw(3, 0);

	direct3DManager->ClearDeviceTargetsAndResources();
}

void SDSMShadowManager::BoxBlurPass(Direct3DManager *direct3DManager, ID3D11ShaderResourceView* input, ID3D11RenderTargetView* output, unsigned int partitionIndex,
	ID3D11ShaderResourceView* partitionSRV, const D3D11_VIEWPORT* viewport, unsigned int dimension, Scene &scene)
{
	direct3DManager->SetupForFullScreenTriangle();
	direct3DManager->UseDefaultRasterState();
	direct3DManager->DisableAlphaBlending();
	direct3DManager->GetDeviceContext()->RSSetViewports(1, viewport);

	direct3DManager->GetDeviceContext()->OMSetRenderTargets(1, &output, 0);

	ShaderParams shaderParams;
	shaderParams.BlurTexture = input;
	shaderParams.PartitionResourceView = partitionSRV;
	shaderParams.FilterSize = Vector2::FromD3DVector(mBlurSizeLightSpace);
	shaderParams.BlurDirection = dimension == 0;
	shaderParams.PartitionIndex = partitionIndex;

	mShadowMapBlurMaterial->SetShaderParameters(direct3DManager->GetDeviceContext(), shaderParams);
	mShadowMapBlurMaterial->Render(direct3DManager->GetDeviceContext());
	direct3DManager->GetDeviceContext()->Draw(3, 0);

	direct3DManager->ClearDeviceTargetsAndResources();
}


void SDSMShadowManager::BoxBlur(Direct3DManager *direct3DManager, unsigned int partitionIndex, ID3D11ShaderResourceView* partitionSRV, Scene &scene)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	mShadowEVSMTextures[partitionIndex]->GetRenderTargetTexture()->GetDesc(&textureDesc);

	D3D11_VIEWPORT viewport;
	viewport.Width = static_cast<float>(textureDesc.Width);
	viewport.Height = static_cast<float>(textureDesc.Height);
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	BoxBlurPass(direct3DManager, mShadowEVSMTextures[partitionIndex]->GetShaderResourceView(0), mShadowEVSMBlurTexture->GetRenderTargetView(0),
		partitionIndex, partitionSRV, &viewport, 0, scene);

	BoxBlurPass(direct3DManager, mShadowEVSMBlurTexture->GetShaderResourceView(0), mShadowEVSMTextures[partitionIndex]->GetRenderTargetView(0),
		partitionIndex, partitionSRV, &viewport, 1, scene);
}
*/