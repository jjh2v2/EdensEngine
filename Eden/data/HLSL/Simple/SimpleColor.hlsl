struct VertexInput
{
    float4 position  : POSITION;
    float4 texCoord0 : TEXCOORD0;
	float3 normal    : NORMAL;
	float3 tangent   : TANGENT;
    float3 binormal  : BINORMAL;
	float4 color     : COLOR;
};

struct VertexOutput
{
    float4 position  	: SV_POSITION;
    float4 texCoord0 	: TEXCOORD0;
	float3 normal    	: NORMAL;
};

Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);

cbuffer MatrixBuffer : register(b0)
{
	matrix worldMatrix;
	matrix viewMatrix;
    matrix projectionMatrix;
};

VertexOutput SimpleColorVertexShader(VertexInput input)
{
    VertexOutput output;
    input.position.w = 1.0f;
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);	
	
    output.texCoord0 = input.texCoord0;
    output.normal = normalize(mul(input.normal, (float3x3)worldMatrix));
	output.normal = normalize(mul(float4(output.normal, 0.0f), viewMatrix).xyz);

    return output;
}

float4 SimpleColorPixelShader(VertexOutput input) : SV_TARGET
{
    float4 textureColor = DiffuseTexture.Sample(DiffuseSampler, input.texCoord0.xy);
	textureColor.rgb = pow(abs(textureColor.rgb), 2.2);	//gamma correction
	
	float3 normal = input.normal;

    return float4(textureColor.rgb, 1);
}