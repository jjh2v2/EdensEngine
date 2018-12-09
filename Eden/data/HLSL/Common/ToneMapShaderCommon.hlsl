#include "../Common/ShaderCommon.hlsl"

static const uint TONEMAP_ACES   = 0;
static const uint TONEMAP_HEJ    = 1;
static const uint TONEMAP_FILMIC = 2;
static const uint TONEMAP_HABLE  = 3;

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
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

float3 TonemapACES(float3 color)
{
    color = RRTAndODTFit(mul(ACESInputMat, color));
    return saturate(mul(ACESOutputMat, color));
}

float CalcLuminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float GetAverageLuminance(Texture2D luminanceTexure)
{
	return luminanceTexure.Load(uint3(0,0,0)).r;
}

float3 CalcExposedColor(float3 color, float avgLuminance, float threshold)
{
    float keyValue = 0.18;
    
    avgLuminance = max(avgLuminance, 0.001f);
    float linearExposure = (keyValue / avgLuminance);
    float exposure = log2(max(linearExposure, 0.0001f)); //use static variable here for manual exposure

    exposure -= threshold;
    return max(exp2(exposure) * color, EPSILON);
}

float3 ToneMapFilmicALU(float3 color)
{
    color = max(0, color - 0.004f);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f)+ 0.06f);

    // result has 1/2.2 baked in, reverse it if using SRGB backbuffer
    return pow(color, 2.2f);
}



float3 CalculateHable(float3 color) 
{
    float A = 0.22;//ShoulderStrength;
    float B = 0.3;//LinearStrength;
    float C = 0.1;//LinearAngle;
    float D = 0.2;//ToeStrength;
    float E = 0.01;//ToeNumerator;
    float F = 0.3;//ToeDenominator;
    
    //const float A = 4.0;        //shoulder strength
    //const float B = 5.0;        //linear strength
    //const float C = 0.12;       //linear angle
    //const float D = 13.0;       //toe strength
    //const float E = 0.01f;
    //const float F = 0.3f;

    return ((color * (A * color + C * B)+ D * E) / (color * (A * color + B) + D * F)) - E / F;
}

float3 ToneMapHable(float3 color) 
{
    return CalculateHable(color) / CalculateHable(6.0); //6.0 = hable whitepoint
}

float3 ToneMapHej(float3 color)
{
    float4 vh = float4(color, 1.0); // 1.0 = hej whitepoint
    float4 va = (1.435f * vh) + 0.05;
    float4 vf = ((vh * va + 0.004f) / ((vh * (va + 0.55f) + 0.0491f))) - 0.0821f;
    return vf.xyz / vf.www;
}

float3 ToneMap(float3 color, float avgLuminance, float threshold, uint toneMapMode)
{
    float3 finalColor = 0;
    color = CalcExposedColor(color, avgLuminance, threshold);

    if(toneMapMode == TONEMAP_ACES)
    {
        const float acesConstant = 1.8;
        finalColor = TonemapACES(acesConstant * color);
    }
    else if(toneMapMode == TONEMAP_HEJ)
    {
        finalColor = ToneMapHej(color);
    }
    else if(toneMapMode == TONEMAP_FILMIC)
    {
        finalColor = ToneMapFilmicALU(color);
    }
    else if(toneMapMode == TONEMAP_HABLE)
    {
        finalColor = ToneMapHable(color);
        finalColor = finalColor / ToneMapHable(0.9);
    }

    return finalColor;
}



