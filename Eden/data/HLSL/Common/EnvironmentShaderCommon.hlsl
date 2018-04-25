#include "../Common/ShaderCommon.hlsl"
#define ENV_MAP_SAMPLES 1024
#define BRDF_SAMPLES 512
#define FILTER_COMPUTE_DIMENSION 8

float RadicalInverseVdC(uint bits) 
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint n) 
{
	return float2(float(i) / float(n), RadicalInverseVdC(i));
}

float3 ImportanceSampleGGX(float2 xi, float roughness, float3 normal)
{
	float a = roughness * roughness;
	float phi = 2 * PI * xi.x;
	float cosTheta = sqrt((1 - xi.y) / (1 + (a*a - 1) * xi.y));
	float sinTheta = sqrt(1 - cosTheta * cosTheta);

	float3 h;
	h.x = sinTheta * cos(phi);
	h.y = sinTheta * sin(phi);
	h.z = cosTheta;

	float3 upVector = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 tangentX = normalize(cross(upVector, normal));
	float3 tangentY = cross(normal, tangentX);
	// Tangent to world space
	return tangentX * h.x + tangentY * h.y + normal * h.z;
}

float4 ImportanceSampleGGX(float2 e, float roughness)
{
	float m = roughness * roughness;
	float m2 = m * m;
	float phi = 2 * PI * e.x;
    
	float cosTheta = sqrt((1 - e.y) / (1 + (m2 - 1) * e.y));
	float sinTheta = sqrt(1 - cosTheta * cosTheta);
    
	float3 h;
	h.x = sinTheta * cos(phi);
	h.y = sinTheta * sin(phi);
	h.z = cosTheta;
    
	float d = (cosTheta * m2 - cosTheta) * cosTheta + 1;
	float dd = m2 / (PI * d * d);
	float PDF = dd * cosTheta;
    
	return float4(h, PDF);
}

float G1Smith(float k, float nDotV)
{
	return nDotV / (nDotV * (1.0 - k) + k);
}

float GSmith(float r, float nDotV, float nDotL)
{
	float r2 = (r + 1) * (r + 1);
	float k = r2 / 8.0;
	return G1Smith(k, nDotV) * G1Smith(k, nDotL);
}

float2 IntegrateBRDF(float roughness, float nDotV)
{
	float3 v;
	v.x = sqrt(1.0f - nDotV * nDotV); // sin
	v.y = 0;
	v.z = nDotV; // cos
    
	float a = 0;
	float b = 0;

	for (uint i = 0; i < BRDF_SAMPLES; i++)
	{
		float2 xi = Hammersley(i, BRDF_SAMPLES);
		float3 h = ImportanceSampleGGX(xi.xy, roughness).xyz;
		float3 l = 2 * dot(v, h) * h - v;
		float nDotL = saturate(l.z);
		float nDotH = saturate(h.z);
		float vDotH = saturate(dot(v, h));
        
		if (nDotL > 0)
		{
			float g = GSmith(roughness, nDotV, nDotL);
			float gVis = g * vDotH / (nDotH * nDotV);
			float fc = pow(1 - vDotH, 5);
            
			a += (1 - fc) * gVis;
			b += fc * gVis;
		}
	}
	return float2(a, b) / BRDF_SAMPLES;
}