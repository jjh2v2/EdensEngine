static const float PI = 3.14159265;

float2 EncodeSphereMap(float3 n)
{
    float oneMinusZ = 1.0f - n.z;
    float p = sqrt(n.x * n.x + n.y * n.y + oneMinusZ * oneMinusZ);
	p = max(p, 0.00001);

    return n.xy * rcp(p) * 0.5f + 0.5f;
}

float3 DecodeSphereMap(float2 e)
{
    float2 tmp = e - e * e;
    float f = tmp.x + tmp.y;
    float m = sqrt(4.0f * f - 1.0f);
    
    float3 n;
    n.xy = m * (e * 4.0f - 2.0f);
    n.z  = 3.0f - 8.0f * f;
	
    return n;
}

float3 GetViewPosition(float2 screenPosition, float viewSpaceDepth, matrix cameraProj)
{
    float3 viewPosition;
    viewPosition.z = viewSpaceDepth;
    viewPosition.xy = float2(screenPosition.x / cameraProj._11, screenPosition.y / cameraProj._22) * viewPosition.z;
    
    return viewPosition;
}

float3 GetViewPosition(uint2 coords, float2 bufferDimensions, float zDepth, matrix cameraProj)
{
    float2 pixelOffset = float2(2.0f, -2.0f) / bufferDimensions;
    float2 positionScreen = (coords.xy + 0.5f) * pixelOffset.xy + float2(-1.0f, 1.0f);
	float viewSpaceDepth = cameraProj._43 / (zDepth - cameraProj._33);
	
	return GetViewPosition(positionScreen , viewSpaceDepth, cameraProj);
}

float3 ViewPositionToLightTexCoord(float3 positionView, matrix cameraViewToLightProj)
{
    float4 positionLight = mul(float4(positionView, 1.0f), cameraViewToLightProj);
    return (positionLight.xyz / positionLight.w) * float3(0.5f, -0.5f, 1.0f) + float3(0.5f, 0.5f, 0.0f);
}