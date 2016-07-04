#include "Consts/VertexShaderConsts.hlsl"
#include "Consts/SharedShaderConsts.hlsl"

VS_to_PS UberLitVertexShader(VertexInput input)
{
    VS_to_PS output;
    input.position.w = 1.0f;

    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
	output.deferredPositionView = output.position;
    output.position = mul(output.position, projectionMatrix);	
	
	output.mvpPosition = output.position;
	
    output.texCoord0 = input.texCoord0;

    float3 worldNormal = normalize(mul(input.normal, (float3x3)worldMatrix));
	output.deferredNormal = mul(float4(worldNormal, 0.0f), viewMatrix).xyz;
	
    float4 worldPosition = mul(input.position, worldMatrix);
    output.viewDirection = normalize(cameraPosition.xyz - worldPosition.xyz);
	
#ifdef USE_NORMAL_MAP
	output.tangent = mul(input.tangent, (float3x3)worldMatrix);
	output.tangent = mul(output.tangent, viewMatrix);
    output.tangent = normalize(output.tangent);

    output.binormal = mul(input.binormal, (float3x3)worldMatrix);
	output.binormal = mul(output.binormal, viewMatrix);
    output.binormal = normalize(output.binormal);
#endif

    return output;
}