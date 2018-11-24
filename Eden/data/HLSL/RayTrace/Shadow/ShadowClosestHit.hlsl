#include "ShadowCommon.hlsl"

[shader("closesthit")] 
void ShadowClosestHit(inout HitInfo payload, Attributes attrib) 
{
    payload.shadowDistance = RayTCurrent();
}
