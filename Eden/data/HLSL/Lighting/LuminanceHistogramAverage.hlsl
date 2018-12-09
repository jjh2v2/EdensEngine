#define HISTOGRAM_AVERAGE_THREADS_PER_DIMENSION 8
#define NUM_HISTOGRAM_BINS 64

RWByteAddressBuffer LuminanceHistogram : register(u0);
RWTexture2D<float> LuminanceOutput : register(u1);

cbuffer LuminanceHistogramAverageBuffer : register(b0)
{
    float luminanceMaxMinusMin;
    float darknessImportanceFactor;
    float darknessImportanceExponent;
    float brightnessImportanceFactor;
    float brightnessImportanceExponent;
    float darknessScalingMax;
    float brightnessScalingMin;
    float tau;
    float timeDelta;
    float luminanceScalar;
};

groupshared float2 HistogramShared[NUM_HISTOGRAM_BINS];

[numthreads(HISTOGRAM_AVERAGE_THREADS_PER_DIMENSION, HISTOGRAM_AVERAGE_THREADS_PER_DIMENSION, 1)]
void LuminanceHistogramAverage(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex, uint3 threadId : SV_DispatchThreadID)
{
    uint histogramBinCount = LuminanceHistogram.Load(groupIndex * 4);
    float luminancePerBin = luminanceMaxMinusMin / NUM_HISTOGRAM_BINS;
    float luminanceForThisBin = luminancePerBin * groupIndex; 
    
    float darknessFactor = lerp(darknessImportanceFactor, 1.0, saturate(groupIndex / darknessScalingMax));
    float brightnessFactor = lerp(1.0, brightnessImportanceFactor, saturate((groupIndex - brightnessScalingMin) / (NUM_HISTOGRAM_BINS - brightnessScalingMin - 1)));
    float luminanceBinCountAdjusted = histogramBinCount * pow(abs(darknessFactor), darknessImportanceExponent) * pow(abs(brightnessFactor), brightnessImportanceExponent);
    float luminanceFactor = luminanceForThisBin * luminanceBinCountAdjusted;
            
    HistogramShared[groupIndex] = float2(luminanceFactor, luminanceBinCountAdjusted);
    
    GroupMemoryBarrierWithGroupSync();
    
    [unroll]
	for(uint histogramSampleIndex = (NUM_HISTOGRAM_BINS >> 1); histogramSampleIndex > 0; histogramSampleIndex >>= 1)
    {
		if(groupIndex < histogramSampleIndex)
        {
			HistogramShared[groupIndex] += HistogramShared[groupIndex + histogramSampleIndex];
        }

		GroupMemoryBarrierWithGroupSync();
	}
    
    if(groupIndex == 0)
    {
        float currentLuminance = (HistogramShared[groupIndex].x / HistogramShared[groupIndex].y) * luminanceScalar;
        float luminanceLastFrame = LuminanceOutput[uint2(0, 0)];
        float adaptedLuminance = luminanceLastFrame + (currentLuminance - luminanceLastFrame) * (1 - exp(-timeDelta * tau));
        LuminanceOutput[uint2(0, 0)] = adaptedLuminance;
    }
}
