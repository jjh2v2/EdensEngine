#include "../Common/EnvironmentShaderCommon.hlsl"

RWTexture2D<float2> LookupTexture : register(u0);

[numthreads(FILTER_COMPUTE_DIMENSION, FILTER_COMPUTE_DIMENSION, 1)]
void GenerateEnvironmentMapLookup(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID)
{
	uint2 sourceDimensions = 0;
	LookupTexture.GetDimensions(sourceDimensions.x, sourceDimensions.y);

	uint groupIndex = groupThreadID.y * FILTER_COMPUTE_DIMENSION + groupThreadID.x;
	float2 sampleCoords = float2(dispatchThreadID.xy) / float2(sourceDimensions.xy);
	float2 color = IntegrateBRDF(sampleCoords.x, sampleCoords.y);

	if ((dispatchThreadID.x < sourceDimensions.x) && (dispatchThreadID.y < sourceDimensions.y))
    {
		LookupTexture[dispatchThreadID.xy] = color;
    }
}