#include "PrepassCommon.hlsl"

[shader("miss")]
void PrepassMiss(inout HitInfo payload : SV_RayPayload)
{
    payload.worldPosition = float3(0x7f000000, 0x7f000000, 0x7f000000);
}