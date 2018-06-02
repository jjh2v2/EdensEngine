Texture2D HDRTexture : register(t0);
Texture2D BloomTexture : register(t1);
SamplerState HDRSampler : register(s0);
SamplerState BloomSampler : register(s1);

cbuffer ToneMapBuffer : register(b0)
{
    float  pfBloomMagnitude;
    float  pfBloomExposure;
    float  pfManualExposure;
};

static const float Float16Scale = 0.0009765625f;

static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);
    color = RRTAndODTFit(color);
    color = mul(ACESOutputMat, color);

    return saturate(color);
}

float3 LinearTosRGB(float3 color)
{
    float3 x = color * 12.92f;
    float3 y = 1.055f * pow(saturate(color), 1.0f / 2.4f) - 0.055f;

    float3 clr = color;
    clr.r = color.r < 0.0031308f ? x.r : y.r;
    clr.g = color.g < 0.0031308f ? x.g : y.g;
    clr.b = color.b < 0.0031308f ? x.b : y.b;

    return clr;
}

float3 CalcExposedColor(float3 color, float offset)
{
    float exposure = pfManualExposure - log2(Float16Scale) + offset;
    return exp2(exposure) * color;
}

float3 ToneMap(float3 color, float threshold)
{
    color = CalcExposedColor(color, threshold);

    return LinearTosRGB(ACESFitted(color) * 1.8f);
}

struct VertexOutput
{
    float4 position  : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

VertexOutput ToneMapVertexShader(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.position  = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float4 ToneMapPixelShader(VertexOutput input) : SV_Target
{
    float3 color = HDRTexture.Sample(HDRSampler, input.texCoord0).rgb;
    color += BloomTexture.Sample(BloomSampler, input.texCoord0).xyz * pfBloomMagnitude * exp2(pfBloomExposure);

    return float4(ToneMap(color, 0), 1.0f);
}
