#include "ShadowCommon.hlsl"

[shader("miss")]
void ShadowMiss(inout HitInfo payload : SV_RayPayload)
{
    payload.shadowFactor = 1.0;
}