#include "PrepassCommon.hlsl"

[shader("closesthit")] 
void PrepassClosestHit(inout HitInfo payload, Attributes attrib) 
{
    payload.worldPosition = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
}
