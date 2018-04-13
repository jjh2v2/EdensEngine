Texture2D<float4> SourceMip   : register(t0);
RWTexture2D<float4> TargetMip : register(u0);
SamplerState MipSampler       : register(s0);

cbuffer MipConstants : register(b0)
{
    float2 targetMipTexelSize;
    uint   sourceMipIndex;
};

[numthreads(8, 8, 1)]
void GenerateMip(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float2 mipTexCoord = (dispatchThreadID.xy + 0.5) * targetMipTexelSize;
    TargetMip[dispatchThreadID.xy] = SourceMip.SampleLevel(MipSampler, mipTexCoord, sourceMipIndex);
}
