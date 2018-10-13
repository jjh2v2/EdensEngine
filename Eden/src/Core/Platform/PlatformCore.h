#pragma once
#include <stdint.h>
#include <stdexcept>
#include <d3d12.h>
#include <atlbase.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <assert.h>
#include <d3dcompiler.h>

#define CPU_FRAME_UPDATE_TIME                   16.7f
#define MIN_LOGICAL_CORES                       4
#define FRAME_BUFFER_COUNT					    2
#define RTV_DESCRIPTOR_HEAP_SIZE			    64
#define SRV_DESCRIPTOR_HEAP_SIZE			    4096
#define DSV_DESCRIPTOR_HEAP_SIZE			    32
#define SAMPLER_DESCRIPTOR_HEAP_SIZE		    64
#define RAY_TRACE_DESCRIPTOR_HEAP_SIZE          16
#define RENDER_PASS_DESCRIPTOR_HEAP_MULTIPLE    256
#define MAX_TEXTURE_SUBRESOURCE_COUNT		    512

#define UPLOAD_BUFFER_SIZE					                2048 * 2048 * 32
#define MAX_GPU_UPLOADS						                16
#define MAX_UPLOADS_PROCESSED_PER_BATCH                     5    //TDA: might need more upload allocators to accomodate this
#define D3D12_GPU_VIRTUAL_ADDRESS_NULL                      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN                   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)
#define D3D12_GPU_RESOURCE_STATE_UNKNOWN                    ((D3D12_RESOURCE_STATES)-1)
#define BARRIER_LIMIT						                16
#define UPLOAD_BUFFER_ALIGNMENT				                512	 //upload buffers need to be 512 aligned
#define NUM_STARTING_COMMAND_ALLOCATORS                     10   //TDA: obviously this needs to be intelligently driven rather than hardcoded
#define MAX_ASYNC_COMPUTE_TEXTURES_TO_PROCESS_PER_FRAME     1
#define ALLOW_RAY_TRACING                                   1
#define ENABLE_DEBUG_LAYER                                  1
#define ENABLE_PIX                                          1

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
	( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
	| D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
	| D3D12_RESOURCE_STATE_COPY_DEST \
	| D3D12_RESOURCE_STATE_COPY_SOURCE )

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
    static char *RayShaderManifestFileLocation;
	static bool  RebuildAllShaders;
	static bool  RebuildAllMeshes;
};

class Direct3DUtils
{
public:
	static void ThrowIfHRESULTFailed(HRESULT hr);
	static void ThrowRuntimeError(char *errorMessage);
    static void ThrowLogicError(char *errorMessage);
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