#pragma once
#include <stdint.h>
#include <stdexcept>
#include <d3d12.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <assert.h>
#include <d3dcompiler.h>

#define NUM_DEFAULT_WORKER_THREADS			8
#define DEFAULT_BUFFERING_COUNT				2
#define RTV_DESCRIPTOR_HEAP_SIZE			64
#define SRV_DESCRIPTOR_HEAP_SIZE			1024
#define DSV_DESCRIPTOR_HEAP_SIZE			32
#define SAMPLER_DESCRIPTOR_HEAP_SIZE		64
#define MAX_TEXTURE_SUBRESOURCE_COUNT		512

#define UPLOAD_BUFFER_SIZE					1024 * 1024 * 32
#define MAX_GPU_UPLOADS						16

typedef int8_t      int8;
typedef int16_t		int16;
typedef int32_t		int32;
typedef int64_t		int64;
typedef uint8_t     uint8;
typedef uint16_t	uint16;
typedef uint32_t	uint32;
typedef uint64_t	uint64;

class ApplicationSpecification
{
public:
	static bool  ForceAllTexturesToSRGB;
	static char* TextureManifestFileLocation;
	static bool  RebuildAllShaders;
};

class Direct3DUtils
{
public:
	static void ThrowIfHRESULTFailed(HRESULT hr);
	static void ThrowRuntimeError(char *errorMessage);
	static void OutputShaderCompileError(ID3DBlob *shaderErrorBlob);
};

class Application
{
public:
	static void Assert(bool param);
	static uint64 Align(uint64 size, uint64 alignment);
};