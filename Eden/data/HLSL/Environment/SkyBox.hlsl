struct VertexInput
{
    float4 position : POSITION;
};

struct VertexOutput
{
    float4 position         : SV_POSITION;
    float3 skyBoxPosition   : TEXCOORD0;
};

TextureCube  EnvironmentMap1     : register(t0);
TextureCube  EnvironmentMap2     : register(t1);
SamplerState EnvironmentSampler  : register(s0);

cbuffer SkyBoxBuffer : register(b0)
{
    matrix pfWVPMatrix;
    float  pfFade;
};

VertexOutput SkyBoxVertexShader(VertexInput input)
{
    VertexOutput output;
    input.position.w = 1.0f;
    output.skyBoxPos = input.position;
    output.position = mul(input.position, pfWVPMatrix);
    
    return output;
}

float4 SkyBoxPixelShader(VertexOutput input): SV_TARGET
{
    float4 color;
	color = pow(EnvironmentMap1.Sample(EnvironmentSampler, normalize(input.skyBoxPos.xyz)), 2.2);
	color = lerp(color, pow(EnvironmentMap2.Sample(EnvironmentSampler, normalize(input.skyBoxPos.xyz)), 2.2), pfFade);
	color.a = 1.0;
    
    return color;
}