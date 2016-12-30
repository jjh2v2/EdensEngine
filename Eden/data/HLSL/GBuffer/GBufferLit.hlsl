#include "../Common/GBufferShaderCommon.hlsl"

GBufferVertexOutput GBufferLitVertexShader(VertexInput input)
{
    GBufferVertexOutput output;
    input.position.w = 1.0f;
    output.position = mul(input.position, poWorldMatrix);
    output.position = mul(output.position, pfViewMatrix);
	output.positionView = output.position;
    output.position = mul(output.position, pfProjectionMatrix);	
	
    output.texCoord0 = input.texCoord0;
    output.normal = normalize(mul(input.normal, (float3x3)poWorldMatrix));
	output.normal = normalize(mul(float4(output.normal, 0.0f), pfViewMatrix).xyz);
	
	if(poUsesNormalMap)
	{
		output.tangent = mul(input.tangent, (float3x3)poWorldMatrix);
		output.tangent = mul(float4(output.tangent, 0), pfViewMatrix).xyz;
		output.tangent = normalize(output.tangent);
		output.binormal = mul(input.binormal, (float3x3)poWorldMatrix);
		output.binormal = mul(float4(output.binormal, 0), pfViewMatrix).xyz;
		output.binormal = normalize(output.binormal);
	}

    return output;
}

GBufferPixelOutput GBufferLitPixelShader(GBufferVertexOutput input)
{
	float2 modifiedTexCoord = input.texCoord0.xy * poTile;
    float4 textureColor = DiffuseTexture.Sample(DiffuseSampler, modifiedTexCoord);
	textureColor.rgb = pow(abs(textureColor.rgb), 2.2);	//gamma correction
	float4 material = float4(0.0,0.0,0.0,0.0);
	
	float3 normal = float3(0,0,0); //see if there's a big enough visual difference to warrant correcting interpolated normals by normalizing in the pixel shader too
	
	if(poUsesNormalMap)
	{
		float4 bumpMap = (NormalMap.Sample(NormalMapSampler, modifiedTexCoord) * 2.0) - 1.0;
		float3 viewSpaceBinormal = cross( input.normal.xyz, input.tangent );
		float3x3 texSpace = float3x3(input.tangent, viewSpaceBinormal, input.normal);
		normal = normalize(mul(bumpMap.xyz, texSpace));
	}
	else
	{
		normal = input.normal;
	}

	GBufferPixelOutput result;
	result.albedo = textureColor * poDiffuseColor;
	
	if(poUsesRoughMetalMap)
	{
		float2 roughMetal = RoughMetalMap.Sample(RoughMetalMapSampler, modifiedTexCoord).rg;
		result.material.r = max(roughMetal.r, 0.01);
		result.material.g = roughMetal.g;
	}
	else
	{
		result.material.r = max(poRoughness, 0.01);
		result.material.g = poMetalness;
	}

	result.material.b = poMaterialIntensity;
	result.material.a = 0;
	result.normals = float4(EncodeSphereMap(normal),
                            ddx_coarse(input.positionView.z),
                            ddy_coarse(input.positionView.z));
    return result;
}