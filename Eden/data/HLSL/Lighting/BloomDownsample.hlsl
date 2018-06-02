Texture2D DownsampleSource : register(t0);
SamplerState BloomSampler  : register(s0);

struct VertexOutput
{
    float4 position  : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

VertexOutput BloomDownsampleVertexShader(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.position  = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float4 BloomDownsamplePixelShader(VertexOutput input) : SV_Target
{
    float4 reds   = DownsampleSource.GatherRed(BloomSampler,   input.texCoord0);
    float4 greens = DownsampleSource.GatherGreen(BloomSampler, input.texCoord0);
    float4 blues  = DownsampleSource.GatherBlue(BloomSampler,  input.texCoord0);
    float3 downsampleOutput = 0.0f;

    [unroll]
    for(uint i = 0; i < 4; i++)
    {
        downsampleOutput += float3(reds[i], greens[i], blues[i]);
    }

    downsampleOutput /= 4.0f;
    return float4(downsampleOutput, 1.0f);
}