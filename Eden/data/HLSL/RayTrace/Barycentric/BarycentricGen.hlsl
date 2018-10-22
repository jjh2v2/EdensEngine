#include "BarycentricCommon.hlsl"

RWTexture2D< float4 > RayResultTarget : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

cbuffer RayTracePerFrameCameraBuffer : register(b0)
{
    matrix pfViewInvMatrix;
    matrix pfProjectionInvMatrix;
};

[shader("raygeneration")] 
void BarycentricGen() 
{
    HitInfo payload;
    payload.colorAndDistance = float4(0.0, 0.0, 0.0, 0.0);
  
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;
    
    RayDesc ray;
    ray.Origin = mul(float4(0, 0, 0, 1), pfViewInvMatrix);
    float4 target = mul(float4(d.x, -d.y, 1, 1), pfProjectionInvMatrix);
    ray.Direction = mul(float4(target.xyz, 0), pfViewInvMatrix);
    ray.TMin = 0.001;
    ray.TMax = 100000;
    
    TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);
    
    RayResultTarget[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);
}
