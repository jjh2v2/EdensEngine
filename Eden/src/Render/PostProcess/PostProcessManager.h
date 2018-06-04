#pragma once
#include "Render/Graphics/GraphicsManager.h"

#define BLOOM_BLUR_PASS_COUNT 4

class PostProcessManager
{
public:
    PostProcessManager(GraphicsManager *graphicsManager);
    ~PostProcessManager();

    void RenderPostProcessing(RenderTarget *hdrTarget);

private:
    void ApplyToneMappingAndBloom(RenderTarget *hdrTarget);
    void CopyToBackBuffer(RenderTarget *renderTargetToCopy);

    GraphicsManager *mGraphicsManager;

    RenderTarget *mBloomDownsampleTarget;
    RenderTarget *mBloomBlurTarget;
    RenderTarget *mTonemapTarget;

    ShaderPSO *mBloomDownsampleShader;
    ShaderPSO *mBloomBlurHorizontalShader;
    ShaderPSO *mBloomBlurVerticalShader;
    ShaderPSO *mTonemapShader;

    ConstantBuffer *mBloomBlurBuffer; //not double buffered because it doesn't change frame to frame
    ConstantBuffer *mToneMapBuffer; //not double buffered because it doesn't change frame to frame
};