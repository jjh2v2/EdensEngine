#include "ShadowCommon.hlsl"

[shader("miss")]
void ShadowMiss(inout HitInfo payload : SV_RayPayload)
{
    payload.shadowDistance = 0;
}