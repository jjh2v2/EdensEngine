#pragma once

#include "Render/Texture/Sampler/Sampler.h"
#include "Render/DirectX/Direct3DManager.h"
#include <map>

enum SamplerType
{
	SAMPLER_DEFAULT_ANISO = 0,
	SAMPLER_DEFAULT_POINT_CLAMP = 1,
    SAMPLER_DEFAULT_LINEAR_CLAMP = 2,
    SAMPLER_DEFAULT_POINT_WRAP = 3,
    SAMPLER_DEFAULT_ANISO_16_CLAMP = 4,
    SAMPLER_CMP_LINEAR = 5,
    SAMPLER_DEFAULT_LINEAR_MIRROR = 6,
    SAMPLER_DEFAULT_LINEAR_WRAP = 7,
};

class SamplerManager
{
public:
	SamplerManager(Direct3DManager *direct3DManager);
	~SamplerManager();

	Sampler *GetSampler(SamplerType samplerType) { return mSamplerLookup[samplerType]; }

private:
	Direct3DHeapManager *mHeapManager;
	std::map<SamplerType, Sampler*> mSamplerLookup;
	DynamicArray<Sampler*> mSamplers;
};