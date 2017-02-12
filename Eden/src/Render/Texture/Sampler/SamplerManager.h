#pragma once

#include "Render/Texture/Sampler/Sampler.h"
#include "Render/DirectX/Direct3DManager.h"
#include <map>

enum SamplerType
{
	SAMPLER_DEFAULT_ANISO = 0
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