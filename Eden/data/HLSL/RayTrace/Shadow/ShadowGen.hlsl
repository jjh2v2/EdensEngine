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
    ray.Origin = positionWorld;
    ray.Direction = pfLightDirection;
    ray.TMin = 0.001;
    ray.TMax = 100000;
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    
    ShadowTarget[launchIndex] = payload.shadowDistance;
}
