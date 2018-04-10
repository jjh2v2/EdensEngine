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
        1, mShadowPreferences.ShadowAntiAliasingSamples, 0, 1.0f);

    for (uint32 i = 0; i < SDSM_SHADOW_PARTITION_COUNT; i++)
    {
        mShadowEVSMTextures[i] = contextManager->CreateRenderTarget(mShadowPreferences.ShadowTextureSize, mShadowPreferences.ShadowTextureSize, 
            DXGI_FORMAT_R32G32B32A32_FLOAT, false, 1, 1, 0, 0);
    }

    mShadowEVSMBlurTexture = contextManager->CreateRenderTarget(mShadowPreferences.ShadowTextureSize, mShadowPreferences.ShadowTextureSize,
        DXGI_FORMAT_R32G32B32A32_FLOAT, false, 1, 1, 0);

    for (uint32 i = 0; i < mShadowPreferences.PartitionCount; i++)
    {
        mPartitionIndexBuffers.Add(contextManager->CreateConstantBuffer(sizeof(ShadowRenderPartitionBuffer)));

        ShadowRenderPartitionBuffer partitionBuffer;
        partitionBuffer.partitionIndex = i;

        mPartitionIndexBuffers[i]->SetConstantBufferData(&partitionBuffer, sizeof(ShadowRenderPartitionBuffer));
    }

    mShadowMapViewport.Width = (float)mShadowPreferences.ShadowTextureSize;
    mShadowMapViewport.Height = (float)mShadowPreferences.ShadowTextureSize;
    mShadowMapViewport.MinDepth = 0.0f;
    mShadowMapViewport.MaxDepth = 1.0f;
    mShadowMapViewport.TopLeftX = 0.0f;
    mShadowMapViewport.TopLeftY = 0.0f;
}

SDSMShadowManager::~SDSMShadowManager()
{
    Direct3DContextManager *contextManager = mGraphicsManager->GetDirect3DManager()->GetContextManager();

    for (uint32 i = 0; i < mShadowPreferences.PartitionCount; i++)
    {
        contextManager->FreeConstantBuffer(mPartitionIndexBuffers[i]);
        mPartitionIndexBuffers[i] = NULL;
    }

    mPartitionIndexBuffers.Clear();

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

void SDSMShadowManager::ComputeShadowPartitions(Camera *camera, D3DXMATRIX &lightViewMatrix, D3DXMATRIX &lightProjectionMatrix, DepthStencilTarget *depthStencil, uint64 gbufferPassFence)
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

    uint32 shadowNumSRVDescs = 1;
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();
    RenderPassDescriptorHeap *shadowSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_ShadowCompute, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), shadowNumSRVDescs);

    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, shadowSRVDescHeap->GetHeap());

    ////// Clear partitions
    graphicsContext->TransitionResource(mShadowPartitionBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
    graphicsContext->TransitionResource(mShadowPartitionBoundsBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

    DescriptorHeapHandle shadowPartitionUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionUAVHandle.GetCPUHandle(), mShadowPartitionBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetPipelineState(mClearShadowPartitionsShader);
    graphicsContext->SetRootSignature(NULL, mClearShadowPartitionsShader->GetRootSignature());
    graphicsContext->SetComputeDescriptorTable(0, shadowPartitionUAVHandle.GetGPUHandle());

    graphicsContext->Dispatch(1, 1, 1);

    ////// Clear Partition Bounds
    DescriptorHeapHandle shadowPartitionBoundsUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionBoundsUAVHandle.GetCPUHandle(), mShadowPartitionBoundsBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetPipelineState(mClearShadowPartitionBoundsShader);
    graphicsContext->SetRootSignature(NULL, mClearShadowPartitionBoundsShader->GetRootSignature());
    graphicsContext->SetComputeDescriptorTable(0, shadowPartitionBoundsUAVHandle.GetGPUHandle());

    graphicsContext->Dispatch(1, 1, 1);

    shadowPartitionUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionUAVHandle.GetCPUHandle(), mShadowPartitionBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    DescriptorHeapHandle shadowPartitionSRVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionSRVHandle.GetCPUHandle(), depthStencil->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    DescriptorHeapHandle shadowPartitionCBVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionCBVHandle.GetCPUHandle(), mSDSMBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetPipelineState(mCalculateDepthBufferBoundsShader);
    graphicsContext->SetRootSignature(NULL, mCalculateDepthBufferBoundsShader->GetRootSignature());
    graphicsContext->SetComputeDescriptorTable(0, shadowPartitionUAVHandle.GetGPUHandle());
    graphicsContext->SetComputeDescriptorTable(1, shadowPartitionSRVHandle.GetGPUHandle());
    graphicsContext->SetComputeDescriptorTable(2, shadowPartitionCBVHandle.GetGPUHandle());

    uint32 dispatchWidth  = ((uint32)direct3DManager->GetScreenSize().X + mCurrentSDSMBuffer.reduceTileDim - 1) / mCurrentSDSMBuffer.reduceTileDim;
    uint32 dispatchHeight = ((uint32)direct3DManager->GetScreenSize().Y + mCurrentSDSMBuffer.reduceTileDim - 1) / mCurrentSDSMBuffer.reduceTileDim;

    graphicsContext->Dispatch(dispatchWidth, dispatchHeight, 1);

    ////// Get Partitions From Depth Bounds

    shadowPartitionUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionUAVHandle.GetCPUHandle(), mShadowPartitionBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionCBVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionCBVHandle.GetCPUHandle(), mSDSMBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetPipelineState(mCalculateLogPartitionsFromDepthBoundsShader);
    graphicsContext->SetRootSignature(NULL, mCalculateLogPartitionsFromDepthBoundsShader->GetRootSignature());
    graphicsContext->SetComputeDescriptorTable(0, shadowPartitionUAVHandle.GetGPUHandle());
    graphicsContext->SetComputeDescriptorTable(1, shadowPartitionCBVHandle.GetGPUHandle());

    graphicsContext->Dispatch(1, 1, 1);

    ////// Get Partition Bounds Range From Partitions
    graphicsContext->TransitionResource(mShadowPartitionBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);

    shadowPartitionBoundsUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionBoundsUAVHandle.GetCPUHandle(), mShadowPartitionBoundsBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionSRVHandle = shadowSRVDescHeap->GetHeapHandleBlock(2);
    D3D12_CPU_DESCRIPTOR_HANDLE currentTextureHandle = shadowPartitionSRVHandle.GetCPUHandle();

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentTextureHandle, depthStencil->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    currentTextureHandle.ptr += shadowSRVDescHeap->GetDescriptorSize();
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentTextureHandle, mShadowPartitionBuffer->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionCBVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionCBVHandle.GetCPUHandle(), mSDSMBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetPipelineState(mCalculatePartitionBoundsShader);
    graphicsContext->SetRootSignature(NULL, mCalculatePartitionBoundsShader->GetRootSignature());
    graphicsContext->SetComputeDescriptorTable(0, shadowPartitionBoundsUAVHandle.GetGPUHandle());
    graphicsContext->SetComputeDescriptorTable(1, shadowPartitionSRVHandle.GetGPUHandle());
    graphicsContext->SetComputeDescriptorTable(2, shadowPartitionCBVHandle.GetGPUHandle());

    graphicsContext->Dispatch(dispatchWidth, dispatchHeight, 1);

    ////// Finalize Partition Bounds
    graphicsContext->TransitionResource(mShadowPartitionBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
    graphicsContext->TransitionResource(mShadowPartitionBoundsBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);

    shadowPartitionUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionUAVHandle.GetCPUHandle(), mShadowPartitionBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionSRVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentTextureHandle, mShadowPartitionBoundsBuffer->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionCBVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionCBVHandle.GetCPUHandle(), mSDSMBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetPipelineState(mFinalizePartitionsShader);
    graphicsContext->SetRootSignature(NULL, mFinalizePartitionsShader->GetRootSignature());
    graphicsContext->SetComputeDescriptorTable(0, shadowPartitionUAVHandle.GetGPUHandle());
    graphicsContext->SetComputeDescriptorTable(1, shadowPartitionSRVHandle.GetGPUHandle());
    graphicsContext->SetComputeDescriptorTable(2, shadowPartitionCBVHandle.GetGPUHandle());

    graphicsContext->Dispatch(1, 1, 1);
}



void SDSMShadowManager::RenderShadowMapPartitions(const D3DXMATRIX &lightViewProjMatrix, DynamicArray<SceneEntity*> &shadowEntities)
{
    Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();
    Direct3DQueueManager *queueManager = direct3DManager->GetContextManager()->GetQueueManager();

    //TDA: * mShadowPreferences.PartitionCount <--- is only for testing. Can just fill those cbv descs on the first partition and then reuse them
    uint32 numCBVSRVDescsShadowMap = shadowEntities.CurrentSize() * mShadowPreferences.PartitionCount + mShadowPreferences.PartitionCount + 1; //1 cbv per entity + X partition cbvs + 1 partition srv
    uint32 numCBVSRVDescsEVSM = 1;
    RenderPassDescriptorHeap *shadowSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_ShadowRender, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), numCBVSRVDescsShadowMap + numCBVSRVDescsEVSM);

    ShaderPipelinePermutation shadowMapPermutation(Render_ShadowMap, Target_Depth_Stencil_Only_32_Sample_4, InputLayout_Standard);
    ShaderPipelinePermutation shadowMapEVSMPermutation(Render_Standard_NoDepth, Target_Single_32_NoDepth, InputLayout_Standard);
    ShaderPSO *shadowMapShader = mGraphicsManager->GetShaderManager()->GetShader("ShadowMap", shadowMapPermutation);
    ShaderPSO *shadowMapEVSMShader = mGraphicsManager->GetShaderManager()->GetShader("ShadowMapToEVSM", shadowMapEVSMPermutation);

    graphicsContext->TransitionResource(mShadowPartitionBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);

    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, shadowSRVDescHeap->GetHeap());
    graphicsContext->SetViewport(mShadowMapViewport);
    graphicsContext->SetScissorRect(0, 0, mShadowPreferences.ShadowTextureSize, mShadowPreferences.ShadowTextureSize);

    DescriptorHeapHandle shadowParitionReadBuffer = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowParitionReadBuffer.GetCPUHandle(), mShadowPartitionBuffer->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    DescriptorHeapHandle shadowMapDepthHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowMapDepthHandle.GetCPUHandle(), mShadowDepthTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    DynamicArray<MaterialTextureType> noTexturesForThisPass;
    RenderPassContext shadowPassContext(graphicsContext, shadowSRVDescHeap, noTexturesForThisPass, direct3DManager->GetFrameIndex());

	// Generate raw depth map
    for (uint32 i = 0; i < SDSM_SHADOW_PARTITION_COUNT; i++)
    {
        DescriptorHeapHandle perPassParitionBuffer = shadowSRVDescHeap->GetHeapHandleBlock(1);
        direct3DManager->GetDevice()->CopyDescriptorsSimple(1, perPassParitionBuffer.GetCPUHandle(), mPartitionIndexBuffers[i]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        graphicsContext->SetPipelineState(shadowMapShader);
        graphicsContext->SetRootSignature(shadowMapShader->GetRootSignature(), NULL);
        graphicsContext->SetGraphicsDescriptorTable(1, perPassParitionBuffer.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(2, shadowParitionReadBuffer.GetGPUHandle());

        RenderShadowDepth(i, &shadowPassContext, lightViewProjMatrix, shadowEntities);

        graphicsContext->SetPipelineState(shadowMapEVSMShader);
        graphicsContext->SetRootSignature(shadowMapEVSMShader->GetRootSignature(), NULL);
        graphicsContext->SetGraphicsDescriptorTable(0, perPassParitionBuffer.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(1, shadowMapDepthHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(2, shadowParitionReadBuffer.GetGPUHandle());

        ConvertToEVSM(i);

        if (mShadowPreferences.UseSoftShadows)
        {
            //insert blur support here
        }

        //Generate mips. Need to implement UAV per mip in render target creation.
    }
}

void SDSMShadowManager::RenderShadowDepth(uint32 partitionIndex, RenderPassContext *renderPassContext, const D3DXMATRIX &lightViewProjMatrix, DynamicArray<SceneEntity*> &shadowEntities)
{
    GraphicsContext *graphicsContext = mGraphicsManager->GetDirect3DManager()->GetContextManager()->GetGraphicsContext();

    graphicsContext->TransitionResource(mShadowDepthTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
    graphicsContext->ClearDepthStencilTarget(mShadowDepthTarget->GetDepthStencilViewHandle().GetCPUHandle(), 1.0f, 0);
    graphicsContext->SetDepthStencilTarget(mShadowDepthTarget->GetDepthStencilViewHandle().GetCPUHandle());

    for (uint32 i = 0; i < shadowEntities.CurrentSize(); i++)
    {
        shadowEntities[i]->RenderShadows(renderPassContext, lightViewProjMatrix);
    }
}


void SDSMShadowManager::ConvertToEVSM(uint32 partitionIndex)
{
    GraphicsContext *graphicsContext = mGraphicsManager->GetDirect3DManager()->GetContextManager()->GetGraphicsContext();

    graphicsContext->TransitionResource(mShadowDepthTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
    graphicsContext->TransitionResource(mShadowEVSMTextures[partitionIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    graphicsContext->SetRenderTarget(mShadowEVSMTextures[partitionIndex]->GetRenderTargetViewHandle().GetCPUHandle());

    graphicsContext->DrawFullScreenTriangle();
}

/*
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