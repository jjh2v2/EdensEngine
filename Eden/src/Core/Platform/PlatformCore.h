#pragma once
#include <stdint.h>
#include <stdexcept>
#include <d3d12.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <assert.h>
#include <d3dcompiler.h>

#define NUM_DEFAULT_LOGICAL_CORES           8
#define BACK_BUFFER_COUNT					2
#define RTV_DESCRIPTOR_HEAP_SIZE			64
#define SRV_DESCRIPTOR_HEAP_SIZE			1024
#define DSV_DESCRIPTOR_HEAP_SIZE			32
#define SAMPLER_DESCRIPTOR_HEAP_SIZE		64
#define MAX_TEXTURE_SUBRESOURCE_COUNT		512

#define UPLOAD_BUFFER_SIZE					2048 * 2048 * 32
#define MAX_GPU_UPLOADS						16
#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)
#define D3D12_GPU_RESOURCE_STATE_UNKNOWN    ((D3D12_RESOURCE_STATES)-1)
#define BARRIER_LIMIT						16
#define UPLOAD_BUFFER_ALIGNMENT				512									//upload buffers need to be 512 aligned


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
	static char *TextureManifestFileLocation;
	static char *MeshManifestFileLocation;
	static char *MeshSerializationLocation;
	static char *ShaderManifestFileLocation;
	static bool  RebuildAllShaders;
	static bool  RebuildAllMeshes;
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

class CPUDeviceInfo
{
public:
    static uint32 GetNumberOfLogicalCores();
private:
};