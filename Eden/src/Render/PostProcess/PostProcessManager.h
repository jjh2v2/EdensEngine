#pragma once
#include "Render/Graphics/GraphicsManager.h"

#define BLOOM_BLUR_PASS_COUNT 2

class PostProcessManager
{
public:
    PostProcessManager(GraphicsManager *graphicsManager);
    ~PostProcessManager();

    void RenderPostProcessing(RenderTarget *hdrTarget, float deltaTime);

private:
    void CalculateLuminance(RenderTarget *hdrTarget, float deltaTime);
    void ApplyToneMappingAndBloom(RenderTarget *hdrTarget);
    void CopyToBackBuffer(RenderTarget *renderTargetToCopy);

    GraphicsManager *mGraphicsManager;

    RenderTarget *mBloomThresholdTarget;
    RenderTarget *mHalfScaleTarget;
    RenderTarget *mQuarterScaleTarget;
    RenderTarget *mEighthScaleTarget;
    RenderTarget *mBlurTempTarget;
    RenderTarget *mTonemapCompositeTarget;
    DynamicArray<RenderTarget*> mLuminanceDownSampleTargets;

    ShaderPSO *mInitialLuminanceShader;
    ShaderPSO *mLuminanceDownsampleShader;
    ShaderPSO *mBloomThresholdShader;
    ShaderPSO *mScalingShader;
    ShaderPSO *mBlurHShader;
    ShaderPSO *mBlurVShader;
    ShaderPSO *mToneMapShader;

    uint32 mLuminanceDimensionSize;
    LuminanceBuffer mCurrentLuminanceBuffer;
    ThresholdBuffer mCurrentThresholdBuffer;
    BlurBuffer mCurrentBlurBuffer;

    ConstantBuffer *mLuminanceBuffers[FRAME_BUFFER_COUNT];
    ConstantBuffer *mThresholdBuffers[FRAME_BUFFER_COUNT];
    ConstantBuffer *mBloomBlurBuffers[FRAME_BUFFER_COUNT];

    //ConstantBuffer *mBloomBlurBuffer; //not double buffered because it doesn't change frame to frame
    //ConstantBuffer *mToneMapBuffer; //not double buffered because it doesn't change frame to frame

    bool mToneMapAndBloomEnabled;
};