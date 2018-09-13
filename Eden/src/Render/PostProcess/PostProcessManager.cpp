#include "Render/PostProcess/PostProcessManager.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"

//TDA: Handle screen resize
PostProcessManager::PostProcessManager(GraphicsManager *graphicsManager)
{
    mGraphicsManager = graphicsManager;

    ShaderPipelinePermutation bloomShaderPermutation(Render_Standard_NoDepth, Target_Single_16_NoDepth, InputLayout_Standard);
    ShaderPipelinePermutation toneMapShaderPermutation(Render_Standard_NoDepth, Target_Standard_BackBuffer_NoDepth, InputLayout_Standard);
    ShaderPipelinePermutation computeShaderEmptyPermutation;
    mInitialLuminanceShader = mGraphicsManager->GetShaderManager()->GetShader("InitialLuminance", computeShaderEmptyPermutation);
    mLuminanceDownsampleShader = mGraphicsManager->GetShaderManager()->GetShader("LuminanceDownsample", computeShaderEmptyPermutation);
    mBloomThresholdShader = mGraphicsManager->GetShaderManager()->GetShader("BloomThreshold", bloomShaderPermutation);
    mScalingShader = mGraphicsManager->GetShaderManager()->GetShader("SimpleScaling", bloomShaderPermutation);
    mBlurHShader = mGraphicsManager->GetShaderManager()->GetShader("SimpleBlurH", bloomShaderPermutation);
    mBlurVShader = mGraphicsManager->GetShaderManager()->GetShader("SimpleBlurV", bloomShaderPermutation);
    mToneMapShader = mGraphicsManager->GetShaderManager()->GetShader("ToneMapComposite", toneMapShaderPermutation);

    Vector2 screenSize = mGraphicsManager->GetDirect3DManager()->GetScreenSize();
    Direct3DContextManager *contextManager = mGraphicsManager->GetDirect3DManager()->GetContextManager();
    mBloomThresholdTarget = contextManager->CreateRenderTarget((uint32)(screenSize.X / 2.0f), (uint32)(screenSize.Y / 2.0f), DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
    mHalfScaleTarget = contextManager->CreateRenderTarget((uint32)(screenSize.X / 2.0f), (uint32)(screenSize.Y / 2.0f), DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
    mQuarterScaleTarget = contextManager->CreateRenderTarget((uint32)(screenSize.X / 4.0f), (uint32)(screenSize.Y / 4.0f), DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
    mEighthScaleTarget = contextManager->CreateRenderTarget((uint32)(screenSize.X / 8.0f), (uint32)(screenSize.Y / 8.0f), DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
    mBlurTempTarget = contextManager->CreateRenderTarget((uint32)(screenSize.X / 8.0f), (uint32)(screenSize.Y / 8.0f), DXGI_FORMAT_R16G16B16A16_FLOAT, false, 1, 1, 0);
    mTonemapCompositeTarget = contextManager->CreateRenderTarget((uint32)screenSize.X, (uint32)screenSize.Y, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, false, 1, 1, 0);

    mLuminanceDimensionSize = 16;

    uint32 luminanceDownScaleSizeX = (uint32)screenSize.X;
    uint32 luminanceDownScaleSizeY = (uint32)screenSize.Y;

    do
    {
        luminanceDownScaleSizeX = MathHelper::DivideByMultipleOf(luminanceDownScaleSizeX, mLuminanceDimensionSize);
        luminanceDownScaleSizeY = MathHelper::DivideByMultipleOf(luminanceDownScaleSizeY, mLuminanceDimensionSize);
        mLuminanceDownSampleTargets.Add(contextManager->CreateRenderTarget(luminanceDownScaleSizeX, luminanceDownScaleSizeY, DXGI_FORMAT_R32_FLOAT, true, 1, 1, 0));

    } while (luminanceDownScaleSizeX > 1 || luminanceDownScaleSizeY > 1);

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mLuminanceBuffers[i] = contextManager->CreateConstantBuffer(sizeof(LuminanceBuffer));
    }

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mThresholdBuffers[i] = contextManager->CreateConstantBuffer(sizeof(ThresholdBuffer));
    }

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mBloomBlurBuffers[i] = contextManager->CreateConstantBuffer(sizeof(BlurBuffer));
    }

    for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        mToneMapBuffers[i] = contextManager->CreateConstantBuffer(sizeof(ToneMapBuffer));
    }

    mCurrentLuminanceBuffer.tau = 1.25f;
    mCurrentLuminanceBuffer.timeDelta = 0.0167f;

    mCurrentThresholdBuffer.bloomThreshold = 1.0f;
    mCurrentThresholdBuffer.toneMapMode = 0;

    mCurrentBlurBuffer.blurSigma = 0.8f;

    mCurrentToneMapBuffer.bloomMagnitude = 1.0f;
    mCurrentToneMapBuffer.toneMapMode = 0;

    mToneMapAndBloomEnabled = true;
}

PostProcessManager::~PostProcessManager()
{
    Direct3DContextManager *contextManager = mGraphicsManager->GetDirect3DManager()->GetContextManager();

//     contextManager->FreeConstantBuffer(mToneMapBuffer);
//     contextManager->FreeConstantBuffer(mBloomBlurBuffer);
//     contextManager->FreeRenderTarget(mBloomDownsampleTarget);
//     contextManager->FreeRenderTarget(mBloomBlurTarget);
//     contextManager->FreeRenderTarget(mTonemapTarget);
}

void PostProcessManager::RenderPostProcessing(RenderTarget *hdrTarget, float deltaTime)
{
    if (mToneMapAndBloomEnabled)
    {
        CalculateLuminance(hdrTarget, deltaTime);
        ApplyToneMappingAndBloom(hdrTarget);
        CopyToBackBuffer(mTonemapCompositeTarget);
    }
    else
    {
        CopyToBackBuffer(hdrTarget);
    }
}

void PostProcessManager::CalculateLuminance(RenderTarget *hdrTarget, float deltaTime)
{
    Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();
    Vector2 screenSize = mGraphicsManager->GetDirect3DManager()->GetScreenSize();

    mCurrentLuminanceBuffer.timeDelta = deltaTime;
    mLuminanceBuffers[direct3DManager->GetFrameIndex()]->SetConstantBufferData(&mCurrentLuminanceBuffer, sizeof(LuminanceBuffer));

    graphicsContext->InsertPixBeginEvent(0xFF00FFFF, "Luminance Calculation");

    graphicsContext->TransitionResource(hdrTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
    graphicsContext->TransitionResource(mLuminanceDownSampleTargets[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

    uint32 numSRVsNeeded = 20; //TDA: make this accurate
    RenderPassDescriptorHeap *luminanceSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_PostProcess, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), numSRVsNeeded);

    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, luminanceSRVDescHeap->GetHeap());

    DescriptorHeapHandle initialLuminanceSRVHandle = luminanceSRVDescHeap->GetHeapHandleBlock(1);
    DescriptorHeapHandle initialLuminanceUAVHandle = luminanceSRVDescHeap->GetHeapHandleBlock(1);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, initialLuminanceSRVHandle.GetCPUHandle(), hdrTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, initialLuminanceUAVHandle.GetCPUHandle(), mLuminanceDownSampleTargets[0]->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetPipelineState(mInitialLuminanceShader);
    graphicsContext->SetRootSignature(NULL, mInitialLuminanceShader->GetRootSignature());
    graphicsContext->SetComputeDescriptorTable(0, initialLuminanceSRVHandle.GetGPUHandle());
    graphicsContext->SetComputeDescriptorTable(1, initialLuminanceUAVHandle.GetGPUHandle());

    graphicsContext->Dispatch(mLuminanceDownSampleTargets[0]->GetWidth(), mLuminanceDownSampleTargets[0]->GetHeight(), 1);

    for (uint32 i = 1; i < mLuminanceDownSampleTargets.CurrentSize(); i++)
    {
        graphicsContext->TransitionResource(mLuminanceDownSampleTargets[i - 1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
        graphicsContext->TransitionResource(mLuminanceDownSampleTargets[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

        DescriptorHeapHandle luminanceDownsampleUAVHandle = luminanceSRVDescHeap->GetHeapHandleBlock(2);
        DescriptorHeapHandle luminanceDownsampleCBVHandle = luminanceSRVDescHeap->GetHeapHandleBlock(1);

        D3D12_CPU_DESCRIPTOR_HANDLE currentLumUAVHandle = luminanceDownsampleUAVHandle.GetCPUHandle();
        direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentLumUAVHandle, mLuminanceDownSampleTargets[i - 1]->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        currentLumUAVHandle.ptr += luminanceSRVDescHeap->GetDescriptorSize();
        direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentLumUAVHandle, mLuminanceDownSampleTargets[i]->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        direct3DManager->GetDevice()->CopyDescriptorsSimple(1, luminanceDownsampleCBVHandle.GetCPUHandle(), mLuminanceBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        graphicsContext->SetPipelineState(mLuminanceDownsampleShader);
        graphicsContext->SetRootSignature(NULL, mLuminanceDownsampleShader->GetRootSignature());
        graphicsContext->SetComputeDescriptorTable(0, luminanceDownsampleUAVHandle.GetGPUHandle());
        graphicsContext->SetComputeDescriptorTable(1, luminanceDownsampleCBVHandle.GetGPUHandle());

        graphicsContext->Dispatch(mLuminanceDownSampleTargets[i]->GetWidth(), mLuminanceDownSampleTargets[i]->GetHeight(), 1);
    }

    graphicsContext->InsertPixEndEvent();
}

void PostProcessManager::ApplyToneMappingAndBloom(RenderTarget *hdrTarget)
{
    Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();
    Vector2 screenSize = mGraphicsManager->GetDirect3DManager()->GetScreenSize();

    D3D12_VIEWPORT fullViewPort    = { 0.0f, 0.0f, screenSize.X,        screenSize.Y,        0.0f, 1.0f };
    D3D12_VIEWPORT halfViewPort    = { 0.0f, 0.0f, screenSize.X / 2.0f, screenSize.Y / 2.0f, 0.0f, 1.0f };
    D3D12_VIEWPORT quarterViewPort = { 0.0f, 0.0f, screenSize.X / 4.0f, screenSize.Y / 4.0f, 0.0f, 1.0f };
    D3D12_VIEWPORT eighthViewPort  = { 0.0f, 0.0f, screenSize.X / 8.0f, screenSize.Y / 8.0f, 0.0f, 1.0f };

    mThresholdBuffers[direct3DManager->GetFrameIndex()]->SetConstantBufferData(&mCurrentThresholdBuffer, sizeof(ThresholdBuffer));

    graphicsContext->InsertPixBeginEvent(0xFF00FFFF, "Bloom Threshold");

    graphicsContext->TransitionResource(mLuminanceDownSampleTargets[mLuminanceDownSampleTargets.CurrentSize() - 1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
    graphicsContext->TransitionResource(mBloomThresholdTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

    uint32 numSRVsNeeded = 30; //TDA: make this accurate
    uint32 numSamplersNeeded = 10;
    RenderPassDescriptorHeap *bloomToneMapSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_PostProcess, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), numSRVsNeeded);
    RenderPassDescriptorHeap *bloomToneMapSamplerDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_PostProcess, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, direct3DManager->GetFrameIndex(), numSamplersNeeded);

    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, bloomToneMapSRVDescHeap->GetHeap());
    graphicsContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, bloomToneMapSamplerDescHeap->GetHeap());

    DescriptorHeapHandle bloomThresholdSRVHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(2);
    DescriptorHeapHandle linearSamplerHandle = bloomToneMapSamplerDescHeap->GetHeapHandleBlock(1);
    DescriptorHeapHandle bloomThresholdCBVHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(1);

    D3D12_CPU_DESCRIPTOR_HANDLE currentBloomThresholdHandle = bloomThresholdSRVHandle.GetCPUHandle();
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentBloomThresholdHandle, hdrTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    currentBloomThresholdHandle.ptr += bloomToneMapSRVDescHeap->GetDescriptorSize();
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentBloomThresholdHandle, mLuminanceDownSampleTargets[mLuminanceDownSampleTargets.CurrentSize() - 1]->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    Sampler *linearSampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_LINEAR_CLAMP);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, linearSamplerHandle.GetCPUHandle(), linearSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomThresholdCBVHandle.GetCPUHandle(), mThresholdBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    graphicsContext->SetViewport(fullViewPort);
    graphicsContext->SetScissorRect(0, 0, (uint32)fullViewPort.Width, (uint32)fullViewPort.Height);
    graphicsContext->SetPipelineState(mBloomThresholdShader);
    graphicsContext->SetRootSignature(mBloomThresholdShader->GetRootSignature(), NULL);
    graphicsContext->SetGraphicsDescriptorTable(0, bloomThresholdSRVHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, linearSamplerHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(2, bloomThresholdCBVHandle.GetGPUHandle());

    graphicsContext->DrawFullScreenTriangle();
    graphicsContext->InsertPixEndEvent();

    graphicsContext->InsertPixBeginEvent(0xFF00FFFF, "Bloom Downsample");
    //down to half 
    graphicsContext->TransitionResource(mBloomThresholdTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
    graphicsContext->TransitionResource(mHalfScaleTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    DescriptorHeapHandle bloomDownsampleSourceHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(1);

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomDownsampleSourceHandle.GetCPUHandle(), mBloomThresholdTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetViewport(halfViewPort);
    graphicsContext->SetScissorRect(0, 0, (uint32)halfViewPort.Width, (uint32)halfViewPort.Height);
    graphicsContext->SetRenderTarget(mHalfScaleTarget->GetRenderTargetViewHandle().GetCPUHandle());

    graphicsContext->SetPipelineState(mScalingShader);
    graphicsContext->SetRootSignature(mScalingShader->GetRootSignature(), NULL);

    graphicsContext->SetGraphicsDescriptorTable(0, bloomDownsampleSourceHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, linearSamplerHandle.GetGPUHandle());

    graphicsContext->DrawFullScreenTriangle();

    //down to quarter
    graphicsContext->TransitionResource(mHalfScaleTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
    graphicsContext->TransitionResource(mQuarterScaleTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    bloomDownsampleSourceHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(1);

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomDownsampleSourceHandle.GetCPUHandle(), mHalfScaleTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetViewport(quarterViewPort);
    graphicsContext->SetScissorRect(0, 0, (uint32)quarterViewPort.Width, (uint32)quarterViewPort.Height);
    graphicsContext->SetRenderTarget(mQuarterScaleTarget->GetRenderTargetViewHandle().GetCPUHandle());

    graphicsContext->SetPipelineState(mScalingShader);
    graphicsContext->SetRootSignature(mScalingShader->GetRootSignature(), NULL);

    graphicsContext->SetGraphicsDescriptorTable(0, bloomDownsampleSourceHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, linearSamplerHandle.GetGPUHandle());

    graphicsContext->DrawFullScreenTriangle();

    //down to eighth
    graphicsContext->TransitionResource(mQuarterScaleTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
    graphicsContext->TransitionResource(mEighthScaleTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    bloomDownsampleSourceHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(1);

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomDownsampleSourceHandle.GetCPUHandle(), mQuarterScaleTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetViewport(eighthViewPort);
    graphicsContext->SetScissorRect(0, 0, (uint32)eighthViewPort.Width, (uint32)eighthViewPort.Height);
    graphicsContext->SetRenderTarget(mEighthScaleTarget->GetRenderTargetViewHandle().GetCPUHandle());

    graphicsContext->SetPipelineState(mScalingShader);
    graphicsContext->SetRootSignature(mScalingShader->GetRootSignature(), NULL);

    graphicsContext->SetGraphicsDescriptorTable(0, bloomDownsampleSourceHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, linearSamplerHandle.GetGPUHandle());

    graphicsContext->DrawFullScreenTriangle();
    graphicsContext->InsertPixEndEvent();

    graphicsContext->InsertPixBeginEvent(0xFF00FFFF, "Bloom Blur");
    mBloomBlurBuffers[direct3DManager->GetFrameIndex()]->SetConstantBufferData(&mCurrentBlurBuffer, sizeof(BlurBuffer));

    DescriptorHeapHandle bloomBlurSourceHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(1);
    DescriptorHeapHandle bloomBlurTargetHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(1);
    DescriptorHeapHandle bloomBlurSamplerHandle = bloomToneMapSamplerDescHeap->GetHeapHandleBlock(1);
    DescriptorHeapHandle bloomBlurCBVHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(1);
    Sampler *pointSampler = mGraphicsManager->GetSamplerManager()->GetSampler(SAMPLER_DEFAULT_POINT_CLAMP);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomBlurSourceHandle.GetCPUHandle(), mEighthScaleTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomBlurTargetHandle.GetCPUHandle(), mBlurTempTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomBlurSamplerHandle.GetCPUHandle(), pointSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomBlurCBVHandle.GetCPUHandle(), mBloomBlurBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    for (uint32 i = 0; i < BLOOM_BLUR_PASS_COUNT; i++)
    {
        //horizontal
        graphicsContext->TransitionResource(mEighthScaleTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
        graphicsContext->TransitionResource(mBlurTempTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

        graphicsContext->SetViewport(eighthViewPort);
        graphicsContext->SetScissorRect(0, 0, (uint32)eighthViewPort.Width, (uint32)eighthViewPort.Height);
        graphicsContext->SetRenderTarget(mBlurTempTarget->GetRenderTargetViewHandle().GetCPUHandle());

        graphicsContext->SetPipelineState(mBlurHShader);
        graphicsContext->SetRootSignature(mBlurHShader->GetRootSignature(), NULL);
         
        graphicsContext->SetGraphicsDescriptorTable(0, bloomBlurSourceHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(1, bloomBlurSamplerHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(2, bloomBlurCBVHandle.GetGPUHandle());
         
        graphicsContext->DrawFullScreenTriangle();

        //vertical
        graphicsContext->TransitionResource(mBlurTempTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
        graphicsContext->TransitionResource(mEighthScaleTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

        graphicsContext->SetViewport(eighthViewPort);
        graphicsContext->SetScissorRect(0, 0, (uint32)eighthViewPort.Width, (uint32)eighthViewPort.Height);
        graphicsContext->SetRenderTarget(mBlurTempTarget->GetRenderTargetViewHandle().GetCPUHandle());

        graphicsContext->SetPipelineState(mBlurVShader);
        graphicsContext->SetRootSignature(mBlurVShader->GetRootSignature(), NULL);

        graphicsContext->SetGraphicsDescriptorTable(0, bloomBlurTargetHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(1, bloomBlurSamplerHandle.GetGPUHandle());
        graphicsContext->SetGraphicsDescriptorTable(2, bloomBlurCBVHandle.GetGPUHandle());

        graphicsContext->DrawFullScreenTriangle();
    }

    graphicsContext->InsertPixEndEvent();

    graphicsContext->InsertPixBeginEvent(0xFF00FFFF, "Bloom Upsample");

    //up to quarter
    graphicsContext->TransitionResource(mEighthScaleTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
    graphicsContext->TransitionResource(mQuarterScaleTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    bloomDownsampleSourceHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(1);

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomDownsampleSourceHandle.GetCPUHandle(), mEighthScaleTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetViewport(quarterViewPort);
    graphicsContext->SetScissorRect(0, 0, (uint32)quarterViewPort.Width, (uint32)quarterViewPort.Height);
    graphicsContext->SetRenderTarget(mQuarterScaleTarget->GetRenderTargetViewHandle().GetCPUHandle());

    graphicsContext->SetPipelineState(mScalingShader);
    graphicsContext->SetRootSignature(mScalingShader->GetRootSignature(), NULL);

    graphicsContext->SetGraphicsDescriptorTable(0, bloomDownsampleSourceHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, linearSamplerHandle.GetGPUHandle());

    graphicsContext->DrawFullScreenTriangle();

    //up to half
    graphicsContext->TransitionResource(mQuarterScaleTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
    graphicsContext->TransitionResource(mHalfScaleTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    bloomDownsampleSourceHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(1);

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomDownsampleSourceHandle.GetCPUHandle(), mQuarterScaleTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetViewport(halfViewPort);
    graphicsContext->SetScissorRect(0, 0, (uint32)halfViewPort.Width, (uint32)halfViewPort.Height);
    graphicsContext->SetRenderTarget(mHalfScaleTarget->GetRenderTargetViewHandle().GetCPUHandle());

    graphicsContext->SetPipelineState(mScalingShader);
    graphicsContext->SetRootSignature(mScalingShader->GetRootSignature(), NULL);

    graphicsContext->SetGraphicsDescriptorTable(0, bloomDownsampleSourceHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, linearSamplerHandle.GetGPUHandle());

    graphicsContext->DrawFullScreenTriangle();

    graphicsContext->InsertPixEndEvent();

    graphicsContext->InsertPixBeginEvent(0xFF00FFFF, "ToneMap Composite");

    graphicsContext->TransitionResource(mHalfScaleTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
    graphicsContext->TransitionResource(mTonemapCompositeTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

    mToneMapBuffers[direct3DManager->GetFrameIndex()]->SetConstantBufferData(&mCurrentToneMapBuffer, sizeof(ToneMapBuffer));

    //tonemap composite
    DescriptorHeapHandle bloomCompositeSRVHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(3);
    DescriptorHeapHandle bloomCompositeCBVHandle = bloomToneMapSRVDescHeap->GetHeapHandleBlock(1);
    D3D12_CPU_DESCRIPTOR_HANDLE currentBloomCompositeSRVHandle = bloomCompositeSRVHandle.GetCPUHandle();

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentBloomCompositeSRVHandle, hdrTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    currentBloomCompositeSRVHandle.ptr += bloomToneMapSRVDescHeap->GetDescriptorSize();
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentBloomCompositeSRVHandle, mLuminanceDownSampleTargets[mLuminanceDownSampleTargets.CurrentSize() - 1]->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    currentBloomCompositeSRVHandle.ptr += bloomToneMapSRVDescHeap->GetDescriptorSize();
    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, currentBloomCompositeSRVHandle, mHalfScaleTarget->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    direct3DManager->GetDevice()->CopyDescriptorsSimple(1, bloomCompositeCBVHandle.GetCPUHandle(), mToneMapBuffers[direct3DManager->GetFrameIndex()]->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    graphicsContext->SetViewport(fullViewPort);
    graphicsContext->SetScissorRect(0, 0, (uint32)fullViewPort.Width, (uint32)fullViewPort.Height);
    graphicsContext->SetRenderTarget(mTonemapCompositeTarget->GetRenderTargetViewHandle().GetCPUHandle());

    graphicsContext->SetPipelineState(mToneMapShader);
    graphicsContext->SetRootSignature(mToneMapShader->GetRootSignature(), NULL);

    graphicsContext->SetGraphicsDescriptorTable(0, bloomCompositeSRVHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(1, linearSamplerHandle.GetGPUHandle());
    graphicsContext->SetGraphicsDescriptorTable(2, bloomCompositeCBVHandle.GetGPUHandle());

    graphicsContext->DrawFullScreenTriangle();

    graphicsContext->InsertPixEndEvent();
}

void PostProcessManager::CopyToBackBuffer(RenderTarget *renderTargetToCopy)
{
    Direct3DManager *direct3DManager = mGraphicsManager->GetDirect3DManager();
    GraphicsContext *graphicsContext = direct3DManager->GetContextManager()->GetGraphicsContext();
    Direct3DHeapManager *heapManager = direct3DManager->GetContextManager()->GetHeapManager();

    BackBufferTarget *backBuffer = direct3DManager->GetBackBufferTarget();

    RenderPassDescriptorHeap *postProcessSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_PostProcess, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, direct3DManager->GetFrameIndex(), 1, !mToneMapAndBloomEnabled);
    RenderPassDescriptorHeap *postProcessSamplerDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_PostProcess, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, direct3DManager->GetFrameIndex(), 1, !mToneMapAndBloomEnabled);
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