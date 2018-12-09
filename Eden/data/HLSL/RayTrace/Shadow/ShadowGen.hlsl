#include "ShadowCommon.hlsl"
#include "../../Common/ShaderCommon.hlsl"

RWTexture2D<float> ShadowTarget : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);
Texture2D DepthTexture : register(t1);

cbuffer ShadowRayTracePerFrameBuffer : register(b0)
{
    matrix pfProjectionMatrix;
    matrix pfViewInvMatrix;
    float4 pfLightDirection;
};

[shader("raygeneration")] 
void ShadowGen() 
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float zDepth = DepthTexture[launchIndex].r;
    float4 positionWorld = mul(float4(GetViewPosition(launchIndex, dims, zDepth, pfProjectionMatrix), 1.0f), pfViewInvMatrix);
    
    HitInfo payload;
    payload.shadowDistance = 0.0;
    
    RayDesc ray;
    ray.Origin = positionWorld.xyz;
    ray.Direction = pfLightDirection.xyz;
    ray.TMin = 0.001;
    ray.TMax = 100000;
    
    float shadowAccum = 0;
    float3 lightDirection = pfLightDirection.xyz;
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    shadowAccum += payload.shadowDistance > EPSILON ? 0.0 : 1;
    lightDirection = normalize(lightDirection + float3(0, 0.015, 0));
    ray.Direction = lightDirection;
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    shadowAccum += payload.shadowDistance > EPSILON ? 0.0 : 1;
    lightDirection = normalize(lightDirection + float3(0, 0.015, 0));
    ray.Direction = lightDirection;
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    shadowAccum += payload.shadowDistance > EPSILON ? 0.0 : 1;
    lightDirection = normalize(lightDirection + float3(0, 0.015, 0));
    ray.Direction = lightDirection;
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    shadowAccum += payload.shadowDistance > EPSILON ? 0.0 : 1;
    lightDirection = normalize(lightDirection + float3(0, 0.015, 0));
    ray.Direction = lightDirection;
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    shadowAccum += payload.shadowDistance > EPSILON ? 0.0 : 1;
    
    ShadowTarget[launchIndex] = shadowAccum/5.0;
}
