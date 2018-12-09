#include "../Common/ToneMapShaderCommon.hlsl"

#define LUMINANCE_THREADS_PER_DIMENSION 16
#define NUM_HISTOGRAM_BINS 64

static const uint LUMINANCE_THREAD_BLOCK_SIZE = LUMINANCE_THREADS_PER_DIMENSION * LUMINANCE_THREADS_PER_DIMENSION;

Texture2D<float4> HDRTexture : register(t0);
RWByteAddressBuffer LuminanceHistogram : register(u0);

cbuffer LuminanceHistogramBuffer : register(b0)
{
    uint inputWidth;
    uint inputHeight;
    float luminanceMin;
    float luminanceMax;
    float luminanceMaxMinusMin;
};

groupshared uint HistogramShared[NUM_HISTOGRAM_BINS];

uint HDRToHistogramBin(float3 hdrColor)
{
    float luminance = max(CalcLuminance(hdrColor), EPSILON);
    float binFactor = saturate((luminance - luminanceMin) / (luminanceMax - luminanceMin));
    return uint(float(NUM_HISTOGRAM_BINS - 1) * binFactor);
}

[numthreads(LUMINANCE_THREADS_PER_DIMENSION, LUMINANCE_THREADS_PER_DIMENSION, 1)]
void LuminanceBuildHistogram(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex, uint3 threadId : SV_DispatchThreadID)
{
    if(groupIndex < NUM_HISTOGRAM_BINS)
    {
        HistogramShared[groupIndex] = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if(threadId.x < inputWidth && threadId.y < inputHeight)
    {
        float3 hdrColor = HDRTexture.Load(int3(threadId.xy, 0)).rgb;
        uint binIndex = HDRToHistogramBin(hdrColor);
        InterlockedAdd(HistogramShared[binIndex], 1);
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if(groupIndex < NUM_HISTOGRAM_BINS)
    {
        LuminanceHistogram.InterlockedAdd(groupIndex * 4, HistogramShared[groupIndex]);
    }
}
