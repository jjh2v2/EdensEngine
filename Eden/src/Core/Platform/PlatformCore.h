#pragma once
#include <stdint.h>
#include <stdexcept>
#include <d3d12.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <assert.h>

#define NUM_DEFAULT_WORKER_THREADS			8
#define DEFAULT_BUFFERING_COUNT				2
#define RTV_DESCRIPTOR_HEAP_SIZE			64
#define SRV_DESCRIPTOR_HEAP_SIZE			1024
#define DSV_DESCRIPTOR_HEAP_SIZE			32
#define SAMPLER_DESCRIPTOR_HEAP_SIZE		64

typedef int16_t		int16;
typedef int32_t		int32;
typedef int64_t		int64;
typedef uint16_t	uint16;
typedef uint32_t	uint32;
typedef uint64_t	uint64;

class ApplicationSpecification
{
public:
	static bool  ForceAllTexturesToSRGB;
	static char* TextureManifestFileLocation;
};

class Direct3DUtils
{
public:
	static void ThrowIfHRESULTFailed(HRESULT hr);
	static void ThrowRuntimeError(char *errorMessage);
};
