#include "Common.hlsl"

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    float3 hitColor = float3(barycentrics.x, barycentrics.y, barycentrics.z);
    
    payload.colorAndDistance = float4(pow(hitColor, 2.2), RayTCurrent());
}
