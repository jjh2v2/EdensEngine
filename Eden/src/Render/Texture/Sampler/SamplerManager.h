#pragma once

#include "Render/Texture/Sampler/Sampler.h"
#include "Render/DirectX/Direct3DManager.h"
#include <map>

enum SamplerType
{
	SAMPLER_DEFAULT_ANISO = 0,
	SAMPLER_DEFAULT_POINT = 1,
    SAMPLER_DEFAULT_LINEAR_POINT = 2
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