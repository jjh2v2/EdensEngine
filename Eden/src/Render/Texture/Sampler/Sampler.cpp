#include "Render/Texture/Sampler/Sampler.h"

Sampler::Sampler(DescriptorHeapHandle samplerHandle)
{
	mSamplerHandle = samplerHandle;
}

Sampler::~Sampler()
{

}