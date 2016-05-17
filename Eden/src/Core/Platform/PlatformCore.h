#pragma once
#include <stdint.h>
#include <stdexcept>
#include <d3d12.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <assert.h>

#define NUM_DEFAULT_WORKER_THREADS 8
#define DEFAULT_BUFFERING_COUNT 2

bool  ForceAllTexturesToSRGB = false;
char* TextureManifestFileLocation = "../Eden/data/Manifests/TextureManifest.emf";

typedef int16_t		int16;
typedef int32_t		int32;
typedef int64_t		int64;
typedef uint16_t	uint16;
typedef uint32_t	uint32;
typedef uint64_t	uint64;

namespace Direct3DUtils
{
	void ThrowIfHRESULTFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw std::runtime_error("Device operation failed.");
		}
	}

	void ThrowRuntimeError(char *errorMessage)
	{
		throw std::runtime_error(errorMessage);
	}
};
