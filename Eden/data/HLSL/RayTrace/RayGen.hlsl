#include "Common.hlsl"

RWTexture2D< float4 > RayResultTarget : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

cbuffer RayTracePerFrameCameraBuffer : register(b0)
{
    matrix pfViewMatrix;
    matrix pfProjectionMatrix;
    matrix pfViewInvMatrix;
    matrix pfProjectionInvMatrix;
};

[shader("raygeneration")] 
void RayGen() 
{
    // Initialize the ray payload
    HitInfo payload;
    payload.colorAndDistance = float4(0.0, 0.0, 0.0, 0.0); //float4(0.9, 0.6, 0.2, 1);
  
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions());
    float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;
    
    // Perspective
    RayDesc ray;
    ray.Origin = mul(pfViewInvMatrix, float4(0, 0, 0, 1));
    float4 target = mul(pfProjectionInvMatrix, float4(d.x, -d.y, 1, 1));
    ray.Direction = mul(pfViewInvMatrix, float4(target.xyz, 0));
    ray.TMin = 0;
    ray.TMax = 100000;
    
    //RayDesc ray;
    //ray.Origin = float3(d.x, -d.y, 1);
    //ray.Direction = float3(0, 0, -1);
    
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    
    RayResultTarget[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);
}
