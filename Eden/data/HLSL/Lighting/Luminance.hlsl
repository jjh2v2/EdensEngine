#include "../Common/ToneMapShaderCommon.hlsl"

#define LUMINANCE_THREADS_PER_DIMENSION 16
static const uint LUMINANCE_THREAD_BLOCK_SIZE = LUMINANCE_THREADS_PER_DIMENSION * LUMINANCE_THREADS_PER_DIMENSION;

Texture2D<float4> HDRTexture : register(t0);
RWTexture2D<float> LuminanceInput : register(u0);
RWTexture2D<float> LuminanceOutput : register(u1);

cbuffer LuminanceBuffer : register(b0)
{
    float tau;
    float timeDelta;
};

groupshared float LuminanceShared[LUMINANCE_THREAD_BLOCK_SIZE];

[numthreads(LUMINANCE_THREADS_PER_DIMENSION, LUMINANCE_THREADS_PER_DIMENSION, 1)]
void InitialLuminance(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
    uint2 textureDimensions;
    HDRTexture.GetDimensions(textureDimensions.x, textureDimensions.y);
    
    uint2 texturePos = min(groupId.xy * LUMINANCE_THREADS_PER_DIMENSION + groupThreadId.xy, textureDimensions - 1);
    float3 hdrColor = HDRTexture[texturePos].xyz;
	LuminanceShared[groupIndex] = log(max(CalcLuminance(hdrColor), EPSILON));
    
	GroupMemoryBarrierWithGroupSync();
    
    [unroll]
	for(uint lumIndex = (LUMINANCE_THREAD_BLOCK_SIZE >> 1); lumIndex > 0; lumIndex >>= 1)
    {
		if(groupIndex < lumIndex)
        {
			LuminanceShared[groupIndex] += LuminanceShared[groupIndex + lumIndex];
        }

		GroupMemoryBarrierWithGroupSync();
	}
    
    if(groupIndex == 0)
    {
        LuminanceOutput[groupId.xy] = (LuminanceShared[0] / LUMINANCE_THREAD_BLOCK_SIZE);
    }
}

[numthreads(LUMINANCE_THREADS_PER_DIMENSION, LUMINANCE_THREADS_PER_DIMENSION, 1)]
void LuminanceDownsample(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
    uint2 textureDimensions;
    LuminanceInput.GetDimensions(textureDimensions.x, textureDimensions.y);

    uint2 texturePos = min(groupId.xy * LUMINANCE_THREADS_PER_DIMENSION + groupThreadId.xy, textureDimensions - 1);
    LuminanceShared[groupIndex] = LuminanceInput[texturePos];
    
    GroupMemoryBarrierWithGroupSync();

    [unroll]
	for(uint lumIndex = (LUMINANCE_THREAD_BLOCK_SIZE >> 1); lumIndex > 0; lumIndex >>= 1)
    {
		if(groupIndex < lumIndex)
        {
			LuminanceShared[groupIndex] += LuminanceShared[groupIndex + lumIndex];
        }

		GroupMemoryBarrierWithGroupSync();
	}

    if(groupIndex == 0)
    {
        uint2 outputDimensions;
        LuminanceOutput.GetDimensions(outputDimensions.x, outputDimensions.y);
    
        if(outputDimensions.x > 1 || outputDimensions.y > 1)
        {
            LuminanceOutput[groupId.xy] = LuminanceShared[0] / LUMINANCE_THREAD_BLOCK_SIZE;
        }
        else
        {
            float luminanceLastFrame = LuminanceOutput[uint2(0, 0)];
            float currentLuminance = exp(LuminanceShared[0] / LUMINANCE_THREAD_BLOCK_SIZE);
            float adaptedLuminance = luminanceLastFrame + (currentLuminance - luminanceLastFrame) * (1 - exp(-timeDelta * tau));
            
            LuminanceOutput[uint2(0, 0)] = adaptedLuminance;
        }
    }
}