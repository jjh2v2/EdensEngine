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

    ShaderPipelinePermutation shadowMapPermutation(Render_ShadowMap, Target_Depth_Stencil_Only_32_Sample_4, InputLayout_Standard);
    ShaderPipelinePermutation shadowMapEVSMPermutation(Render_Standard_NoDepth, Target_Single_32_NoDepth, InputLayout_Standard);
    mShadowMapShader = mGraphicsManager->GetShaderManager()->GetShader("ShadowMap", shadowMapPermutation);
    mShadowMapEVSMShader = mGraphicsManager->GetShaderManager()->GetShader("ShadowMapToEVSM", shadowMapEVSMPermutation);
    mGenerateMipShader = mGraphicsManager->GetShaderManager()->GetShader("GenerateMip", computeShaderEmptyPermutation);

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
            DXGI_FORMAT_R32G32B32A32_FLOAT, true, 1, 1, 0, mShadowPreferences.ShadowTextureMipLevels);
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
    mShadowMapShader = NULL;
    mShadowMapEVSMShader = NULL;
    mGenerateMipShader = NULL;

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
    mCurrentSDSMBuffer.cameraNearFar = Vector2(cameraSettings.Near, cameraSettings.Far);
    mCurrentSDSMBuffer.dilationFactor = 0.01f;
    mCurrentSDSMBuffer.reduceTileDim = 64;

    D3DXMatrixTranspose(&mCurrentSDSMBuffer.cameraProjMatrix, &mCurrentSDSMBuffer.cameraProjMatrix);
    D3DXMatrixTranspose(&mCurrentSDSMBuffer.cameraViewToLightProjMatrix, &mCurrentSDSMBuffer.cameraViewToLightProjMatrix);

    mSDSMBuffers[direct3DManager->GetFrameIndex()]->SetConstantBufferData(&mCurrentSDSMBuffer, sizeof(SDSMBuffer));

    uint32 shadowNumSRVDescs = 1;
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();
    RenderPassDescriptorHeap *shadowSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_ShadowCompute, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), shadowNumSRVDescs);

    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();

    graphicsContext->InsertPixBeginEvent(0xFF00FF00, "Shadow Compute");

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

    graphicsContext->TransitionResource(mShadowPartitionBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);          //force UAV barrier 
    graphicsContext->TransitionResource(mShadowPartitionBoundsBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

    /////// Calculate depth buffer bounds

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

    graphicsContext->Dispatch2D((uint32)direct3DManager->GetScreenSize().X, (uint32)direct3DManager->GetScreenSize().Y, mCurrentSDSMBuffer.reduceTileDim, mCurrentSDSMBuffer.reduceTileDim);

    ////// Get Partitions From Depth Bounds
    graphicsContext->TransitionResource(mShadowPartitionBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

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

    graphicsContext->Dispatch2D((uint32)direct3DManager->GetScreenSize().X, (uint32)direct3DManager->GetScreenSize().Y, mCurrentSDSMBuffer.reduceTileDim, mCurrentSDSMBuffer.reduceTileDim);

    ////// Finalize Partition Bounds
    graphicsContext->TransitionResource(mShadowPartitionBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
    graphicsContext->TransitionResource(mShadowPartitionBoundsBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);

    shadowPartitionUAVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionUAVHandle.GetCPUHandle(), mShadowPartitionBuffer->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionSRVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionSRVHandle.GetCPUHandle(), mShadowPartitionBoundsBuffer->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    shadowPartitionCBVHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowPartitionCBVHandle.GetCPUHandle(), mSDSMBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetPipelineState(mFinalizePartitionsShader);
    graphicsContext->SetRootSignature(NULL, mFinalizePartitionsShader->GetRootSignature());
    graphicsContext->SetComputeDescriptorTable(0, shadowPartitionUAVHandle.GetGPUHandle());
    graphicsContext->SetComputeDescriptorTable(1, shadowPartitionSRVHandle.GetGPUHandle());
    graphicsContext->SetComputeDescriptorTable(2, shadowPartitionCBVHandle.GetGPUHandle());

    graphicsContext->Dispatch(1, 1, 1);

    graphicsContext->InsertPixEndEvent();
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
    uint32 numCBVSRVDescsMips = 1 + mShadowPreferences.ShadowTextureMipLevels;
    RenderPassDescriptorHeap *shadowSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_ShadowRender, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), numCBVSRVDescsShadowMap + numCBVSRVDescsEVSM + numCBVSRVDescsMips);
    RenderPassDescriptorHeap *shadowSamplerDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_ShadowRender, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, direct3DManager->GetFrameIndex(), 1);
    
    graphicsContext->InsertPixBeginEvent(0xFF00FF00, "Shadow Render");

    graphicsContext->TransitionResource(mShadowPartitionBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);

    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, shadowSRVDescHeap->GetHeap());
    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, shadowSamplerDescHeap->GetHeap());
    graphicsContext->SetViewport(mShadowMapViewport);
    graphicsContext->SetScissorRect(0, 0, mShadowPreferences.ShadowTextureSize, mShadowPreferences.ShadowTextureSize);

    DescriptorHeapHandle shadowParitionReadBuffer = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowParitionReadBuffer.GetCPUHandle(), mShadowPartitionBuffer->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    DescriptorHeapHandle shadowMapDepthHandle = shadowSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, shadowMapDepthHandle.GetCPUHandle(), mShadowDepthTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    DynamicArray<MaterialTextureType> noTexturesForThisPass;
    RenderPassContext shadowPassContext(graphicsContext, shadowSRVDescHeap, shadowSamplerDescHeap, noTexturesForThisPass, direct3DManager->GetFrameIndex());

    for (uint32 i = 0; i < SDSM_SHADOW_PARTITION_COUNT; i++)
    {
        DescriptorHeapHandle perPassParitionBuffer = shadowSRVDescHeap->GetHeapHandleBlock(1);
        direct3DManager->GetDevice()->CopyDescriptorsSimple(1, perPassParitionBuffer.GetCPUHandle(), mPartitionIndexBuffers[i]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        graphicsContext->SetPipelineState(mShadowMapShader);
        graphicsContext->SetRootSignature(mShadowMapShader->GetRootSignature(), NULL);
        graphicsContext->SetGraphicsDescriptorTable(1, perPassParitionBuffer.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(2, shadowParitionReadBuffer.GetGPUHandle());

        RenderShadowDepth(i, &shadowPassContext, lightViewProjMatrix, shadowEntities);

        graphicsContext->SetPipelineState(mShadowMapEVSMShader);
        graphicsContext->SetRootSignature(mShadowMapEVSMShader->GetRootSignature(), NULL);
        graphicsContext->SetGraphicsDescriptorTable(0, perPassParitionBuffer.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(1, shadowMapDepthHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(2, shadowParitionReadBuffer.GetGPUHandle());

        ConvertToEVSM(i);

        if (mShadowPreferences.UseSoftShadows)
        {
            ApplyBlur();
        }

        GenerateMipsForShadowMap(i, &shadowPassContext);
        graphicsContext->TransitionResource(mShadowEVSMTextures[i], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);
    }

    graphicsContext->InsertPixEndEvent();
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

void SDSMShadowManager::ApplyBlur()
{
    //insert blur support here
}

void SDSMShadowManager::GenerateMipsForShadowMap(uint32 partitionIndex, RenderPassContext *renderPassContext)
{
    Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();

    graphicsContext->TransitionResourceExplicit(mShadowEVSMTextures[partitionIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0, false, true);

    graphicsContext->SetPipelineState(mGenerateMipShader);
    graphicsContext->SetRootSignature(NULL, mGenerateMipShader->GetRootSignature());

    DescriptorHeapHandle evsmTargetHandle = renderPassContext->GetCBVSRVHeap()->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, evsmTargetHandle.GetCPUHandle(), mShadowEVSMTextures[partitionIndex]->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    Sampler *mipSampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_LINEAR_CLAMP);
    DescriptorHeapHandle samplerHandle = renderPassContext->GetSamplerHeap()->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, samplerHandle.GetCPUHandle(), mipSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    graphicsContext->SetComputeDescriptorTable(2, samplerHandle.GetGPUHandle());
    graphicsContext->SetComputeDescriptorTable(3, evsmTargetHandle.GetGPUHandle());

    uint32 mipTextureSize = mShadowPreferences.ShadowTextureSize;

    for (uint32 i = 1; i < mShadowPreferences.ShadowTextureMipLevels; i++)
    {
        graphicsContext->TransitionResourceExplicit(mShadowEVSMTextures[partitionIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, i, false, true);

        DescriptorHeapHandle mipTargetHandle = renderPassContext->GetCBVSRVHeap()->GetHeapHandleBlock(1);
        direct3DManager->GetDevice()->CopyDescriptorsSimple(1, mipTargetHandle.GetCPUHandle(), mShadowEVSMTextures[partitionIndex]->GetUnorderedAccessViewHandle(i).GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
        mipTextureSize = mipTextureSize >> 1;
        float mipTexelSize = 1.0f / (float)mipTextureSize;
        Vector3 mipConstants = Vector3(mipTexelSize, mipTexelSize, (float)(i - 1));

        graphicsContext->SetComputeDescriptorTable(0, mipTargetHandle.GetGPUHandle());
        graphicsContext->SetComputeConstants(1, 3, &mipConstants);
        graphicsContext->Dispatch2D(mipTextureSize, mipTextureSize, 8, 8);

        graphicsContext->InsertUAVBarrier(mShadowEVSMTextures[partitionIndex], false);

        graphicsContext->TransitionResourceExplicit(mShadowEVSMTextures[partitionIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, i, false, true);
    }

    for (uint32 i = 1; i < mShadowPreferences.ShadowTextureMipLevels; i++)
    {
        graphicsContext->TransitionResourceExplicit(mShadowEVSMTextures[partitionIndex], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, i, false, false);
    }

    graphicsContext->TransitionResourceExplicit(mShadowEVSMTextures[partitionIndex], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, true, true);
}