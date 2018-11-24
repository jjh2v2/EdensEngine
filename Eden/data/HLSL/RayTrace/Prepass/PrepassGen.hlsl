#include "PrepassCommon.hlsl"

RWStructuredBuffer<float4> PositionTarget : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

cbuffer RayTracePerFrameBuffer : register(b0)
{
    matrix pfViewInvMatrix;
    matrix pfProjectionInvMatrix;
};

[shader("raygeneration")] 
void PrepassGen() 
{
    HitInfo payload;
    payload.worldPosition = float3(0.0, 0.0, 0.0);
  
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;
    
    RayDesc ray;
    ray.Origin = mul(float4(0, 0, 0, 1), pfViewInvMatrix).xyz;
    float4 target = mul(float4(d.x, -d.y, 1, 1), pfProjectionInvMatrix);
    ray.Direction = mul(float4(target.xyz, 0), pfViewInvMatrix).xyz;
    ray.TMin = 0.001;
    ray.TMax = 100000;
    
    TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);
    
    PositionTarget[launchIndex.y * dims.x + launchIndex.x] = float4(payload.worldPosition.xyz, 0.0);
}
