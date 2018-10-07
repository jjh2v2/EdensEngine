#include "Common.hlsl"

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float ramp = launchIndex.y / dims.y;
    float3 color = float3(0.0f, 0.2f, 0.7f - 0.6f * ramp);
    
    payload.colorAndDistance = float4(pow(color, 2.2), -1.0f);
}