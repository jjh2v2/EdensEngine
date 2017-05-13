Texture2D ScreenTexture;
SamplerState ScreenTextureSampler;

struct VertexOutput
{
    float4 positionViewport : SV_POSITION;
    float2 texCoord0        : TEXCOORD0;
};

VertexOutput SimpleCopyVertexShader(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.positionViewport = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    
    return output;
}

float4 SimpleCopyPixelShader(VertexOutput input) : SV_TARGET
{
    return ScreenTexture.Sample(ScreenTextureSampler, input.texCoord0);
}