#pragma once

#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/Heap/DescriptorHeapHandle.h"

class Sampler
{
public:
	Sampler(DescriptorHeapHandle samplerHandle);
	~Sampler();

	DescriptorHeapHandle GetSamplerHandle() { return mSamplerHandle; }

private:
	DescriptorHeapHandle mSamplerHandle;
};