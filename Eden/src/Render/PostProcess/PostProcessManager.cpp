#include "Render/PostProcess/PostProcessManager.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"

PostProcessManager::PostProcessManager(GraphicsManager *graphicsManager)
{
    mGraphicsManager = graphicsManager;

    ShaderPipelinePermutation bloomShaderPermutation(Render_Standard_NoDepth, Target_Single_16_NoDepth, InputLayout_Standard);
    ShaderPipelinePermutation toneMapShaderPermutation(Render_Standard_NoDepth, Target_Standard_BackBuffer_NoDepth, InputLayout_Standard);
    mBloomDownsampleShader = mGraphicsManager->GetShaderManager()->GetShader("BloomDownsample", bloomShaderPermutation);
    mBloomBlurHorizontalShader = mGraphicsManager->GetShaderManager()->GetShader("BloomHorizontalBlur", bloomShaderPermutation);
    mBloomBlurVerticalShader = mGraphicsManager->GetShaderManager()->GetShader("BloomVerticalBlur", bloomShaderPermutation);
    mTonemapShader = mGraphicsManager->GetShaderManager()->GetShader("ToneMap", toneMapShaderPermutation);

    Vector2 screenSize = mGraphicsManager->GetDirect3DManager()->GetScreenSize();
    Direct3DContextManager *contextManager = mGraphicsManager->GetDirect3DManager()->GetContextManager();
    mBloomDownsampleTarget = contextManager->CreateRenderTarget((uint32)(screenSize.X / 2.0f), (uint32)(screenSize.Y / 2.0f), DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
    mBloomBlurTarget = contextManager->CreateRenderTarget((uint32)(screenSize.X / 2.0f), (uint32)(screenSize.Y / 2.0f), DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
    mTonemapTarget = contextManager->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, false, 1, 1, 0);

    BloomBlurBuffer blurBuffer;
    blurBuffer.blurInputDimensions = screenSize * 0.5f;
    blurBuffer.blurSigma = 2.5f;

    mBloomBlurBuffer = contextManager->CreateConstantBuffer(sizeof(BloomBlurBuffer));
    mBloomBlurBuffer->SetConstantBufferData(&blurBuffer, sizeof(BloomBlurBuffer));

    ToneMapBuffer toneMapBuffer;
    toneMapBuffer.bloomMagnitude = 1.0f;
    toneMapBuffer.bloomExposure = -4.0f;
    toneMapBuffer.manualExposure = -11.0f;

    mToneMapBuffer = contextManager->CreateConstantBuffer(sizeof(ToneMapBuffer));
    mToneMapBuffer->SetConstantBufferData(&toneMapBuffer, sizeof(ToneMapBuffer));
}

PostProcessManager::~PostProcessManager()
{
    Direct3DContextManager *contextManager = mGraphicsManager->GetDirect3DManager()->GetContextManager();

    contextManager->FreeConstantBuffer(mToneMapBuffer);
    contextManager->FreeConstantBuffer(mBloomBlurBuffer);
    contextManager->FreeRenderTarget(mBloomDownsampleTarget);
    contextManager->FreeRenderTarget(mBloomBlurTarget);
    contextManager->FreeRenderTarget(mTonemapTarget);
}

void PostProcessManager::RenderPostProcessing(RenderTarget *hdrTarget)
{
    ApplyToneMappingAndBloom(hdrTarget);
    CopyToBackBuffer(mTonemapTarget);
}

void PostProcessManager::ApplyToneMappingAndBloom(RenderTarget *hdrTarget)
{
    Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();
    Vector2 screenSize = mGraphicsManager->GetDirect3DManager()->GetScreenSize();

    const uint32 numSRVsNeeded = 10;
    const uint32 numSamplersNeeded = 3;
    RenderPassDescriptorHeap *toneMapSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_PostProcess, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), numSRVsNeeded);
    RenderPassDescriptorHeap *toneMapSamplerDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_PostProcess, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, direct3DManager->GetFrameIndex(), numSamplersNeeded);

    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, toneMapSRVDescHeap->GetHeap());
    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, toneMapSamplerDescHeap->GetHeap());

    D3D12_VIEWPORT halfViewPort = { 0.0f, 0.0f, screenSize.X / 2.0f, screenSize.Y / 2.0f, 0.0f, 1.0f };
    graphicsContext->SetViewport(halfViewPort);
    graphicsContext->SetScissorRect(0, 0, (uint32)(screenSize.X / 2.0f), (uint32)(screenSize.Y / 2.0f));

    //bloom downsample
    DescriptorHeapHandle bloomDownsampleSourceHandle = toneMapSRVDescHeap->GetHeapHandleBlock(1);
    DescriptorHeapHandle bloomDownsampleSamplerHandle = toneMapSamplerDescHeap->GetHeapHandleBlock(1);

    Sampler *linearSampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_LINEAR_CLAMP);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomDownsampleSamplerHandle.GetCPUHandle(), linearSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomDownsampleSourceHandle.GetCPUHandle(), hdrTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetRenderTarget(mBloomDownsampleTarget->GetRenderTargetViewHandle().GetCPUHandle());

    graphicsContext->SetPipelineState(mBloomDownsampleShader);
    graphicsContext->SetRootSignature(mBloomDownsampleShader->GetRootSignature(), NULL);

    graphicsContext->SetGraphicsDescriptorTable(0, bloomDownsampleSourceHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, bloomDownsampleSamplerHandle.GetGPUHandle());

    graphicsContext->DrawFullScreenTriangle();

    //bloom blur
    DescriptorHeapHandle bloomBlurSourceHandle = toneMapSRVDescHeap->GetHeapHandleBlock(1);
    DescriptorHeapHandle bloomBlurTargetHandle = toneMapSRVDescHeap->GetHeapHandleBlock(1);
    DescriptorHeapHandle bloomBlurSamplerHandle = toneMapSamplerDescHeap->GetHeapHandleBlock(1);
    DescriptorHeapHandle bloomBlurCBVHandle = toneMapSRVDescHeap->GetHeapHandleBlock(1);
    Sampler *pointSampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_POINT_CLAMP);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomBlurSourceHandle.GetCPUHandle(), mBloomDownsampleTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomBlurTargetHandle.GetCPUHandle(), mBloomBlurTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomBlurSamplerHandle.GetCPUHandle(), linearSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomBlurCBVHandle.GetCPUHandle(), mBloomBlurBuffer->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (uint32 i = 0; i < BLOOM_BLUR_PASS_COUNT; i++)
    {
        //horizontal
        graphicsContext->SetRenderTarget(mBloomBlurTarget->GetRenderTargetViewHandle().GetCPUHandle());

        graphicsContext->SetPipelineState(mBloomBlurHorizontalShader);
        graphicsContext->SetRootSignature(mBloomBlurHorizontalShader->GetRootSignature(), NULL);

        graphicsContext->SetGraphicsDescriptorTable(0, bloomBlurCBVHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(1, bloomBlurSourceHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(2, bloomBlurSamplerHandle.GetGPUHandle());

        graphicsContext->DrawFullScreenTriangle();

        //vertical
        graphicsContext->SetRenderTarget(mBloomDownsampleTarget->GetRenderTargetViewHandle().GetCPUHandle());

        graphicsContext->SetPipelineState(mBloomBlurVerticalShader);
        graphicsContext->SetRootSignature(mBloomBlurVerticalShader->GetRootSignature(), NULL);

        graphicsContext->SetGraphicsDescriptorTable(0, bloomBlurCBVHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(1, bloomBlurTargetHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(2, bloomBlurSamplerHandle.GetGPUHandle());

        graphicsContext->DrawFullScreenTriangle();
    }

    //tonemap
    DescriptorHeapHandle toneMapTexturesHandle = toneMapSRVDescHeap->GetHeapHandleBlock(2);
    DescriptorHeapHandle toneMapSamplersHandle = toneMapSamplerDescHeap->GetHeapHandleBlock(2);
    DescriptorHeapHandle toneMapCBVHandle = toneMapSRVDescHeap->GetHeapHandleBlock(1);
    D3D12_CPU_DESCRIPTOR_HANDLE toneMapTexturesHandleOffset = toneMapTexturesHandle.GetCPUHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE toneMapSamplersHandleOffset = toneMapSamplersHandle.GetCPUHandle();

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, toneMapTexturesHandleOffset, hdrTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    toneMapTexturesHandleOffset.ptr += toneMapSRVDescHeap->GetDescriptorSize();
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, toneMapTexturesHandleOffset, mBloomDownsampleTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, toneMapSamplersHandleOffset, pointSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    toneMapSamplersHandleOffset.ptr += toneMapSamplerDescHeap->GetDescriptorSize();
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, toneMapSamplersHandleOffset, linearSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, toneMapCBVHandle.GetCPUHandle(), mToneMapBuffer->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetViewport(direct3DManager->GetScreenViewport());
    graphicsContext->SetScissorRect(0, 0, (uint32)screenSize.X, (uint32)screenSize.Y);

    graphicsContext->SetRenderTarget(mTonemapTarget->GetRenderTargetViewHandle().GetCPUHandle());

    graphicsContext->SetPipelineState(mTonemapShader);
    graphicsContext->SetRootSignature(mTonemapShader->GetRootSignature(), NULL);

    graphicsContext->SetGraphicsDescriptorTable(0, toneMapCBVHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, toneMapTexturesHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(2, toneMapSamplersHandle.GetGPUHandle());

    graphicsContext->DrawFullScreenTriangle();

}

void PostProcessManager::CopyToBackBuffer(RenderTarget *renderTargetToCopy)
{
    Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();

    BackBufferTarget *backBuffer = direct3DManager->GetBackBufferTarget();

    RenderPassDescriptorHeap *postProcessSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_PostProcess, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), 1, false);
    RenderPassDescriptorHeap *postProcessSamplerDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_PostProcess, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, direct3DManager->GetFrameIndex(), 1, false);
    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, postProcessSRVDescHeap->GetHeap());
    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, postProcessSamplerDescHeap->GetHeap());

    DescriptorHeapHandle copyTextureHandle = postProcessSRVDescHeap->GetHeapHandleBlock(1);
    DescriptorHeapHandle copySamplerHandle = postProcessSamplerDescHeap->GetHeapHandleBlock(1);

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, copyTextureHandle.GetCPUHandle(), renderTargetToCopy->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, copySamplerHandle.GetCPUHandle(), mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_POINT_CLAMP)->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    graphicsContext->TransitionResource(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
    graphicsContext->TransitionResource(renderTargetToCopy, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);

    graphicsContext->SetRenderTarget(backBuffer->GetRenderTargetViewHandle().GetCPUHandle());
    Vector2 screenSize = direct3DManager->GetScreenSize();
    graphicsContext->SetViewport(direct3DManager->GetScreenViewport());
    graphicsContext->SetScissorRect(0, 0, (uint32)screenSize.X, (uint32)screenSize.Y);

    ShaderPipelinePermutation copyPermutation(Render_Standard_NoDepth, Target_Standard_BackBuffer_NoDepth, InputLayout_Standard);
    ShaderPSO *copyShader = mGraphicsManager->GetShaderManager()->GetShader("SimpleCopy", copyPermutation);
    graphicsContext->SetPipelineState(copyShader);
    graphicsContext->SetRootSignature(copyShader->GetRootSignature(), NULL);

    graphicsContext->SetGraphicsDescriptorTable(0, copyTextureHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, copySamplerHandle.GetGPUHandle());

    graphicsContext->DrawFullScreenTriangle();

    graphicsContext->TransitionResource(backBuffer, D3D12_RESOURCE_STATE_PRESENT, true);
}