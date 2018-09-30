#include "Core/Platform/PlatformCore.h"
#include <assert.h>
#include <fstream>
#include <thread>

bool ApplicationSpecification::ForceAllTexturesToSRGB = false;
char *ApplicationSpecification::TextureManifestFileLocation = "../Eden/data/Manifests/TextureManifest.emf";
char *ApplicationSpecification::MeshManifestFileLocation    = "../Eden/data/Manifests/MeshManifest.emf";
char *ApplicationSpecification::MeshSerializationLocation   = "../Eden/data/Meshes/Serialized/";
char *ApplicationSpecification::ShaderManifestFileLocation  = "../Eden/data/Manifests/ShaderManifest.emf";
bool ApplicationSpecification::RebuildAllShaders = true;
bool ApplicationSpecification::RebuildAllMeshes = false;

void Direct3DUtils::ThrowIfHRESULTFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::runtime_error("Device operation failed.");
	}
}

void Direct3DUtils::ThrowRuntimeError(char *errorMessage)
{
	throw std::runtime_error(errorMessage);
}

void Direct3DUtils::ThrowLogicError(char *errorMessage)
{
    throw std::logic_error(errorMessage);
}

void Direct3DUtils::OutputShaderCompileError(ID3DBlob *shaderErrorBlob)
{
	char* compileErrors = (char*)(shaderErrorBlob->GetBufferPointer());
	SIZE_T bufferSize = shaderErrorBlob->GetBufferSize();
	std::ofstream fileStream;
	fileStream.open("shader-error.txt");

	for (SIZE_T i = 0; i < bufferSize; i++)
	{
		fileStream << compileErrors[i];
	}

	fileStream.close();
	shaderErrorBlob->Release();
	shaderErrorBlob = NULL;
}

void Application::Assert(bool param)
{
	assert(param);
}

 uint64 Application::Align(uint64 size, uint64 alignment)
{
	return ((size + alignment - 1) / alignment) * alignment;
}

 uint32 CPUDeviceInfo::GetNumberOfLogicalCores()
 {
     uint32 reportedNumThreads = std::thread::hardware_concurrency();
     return (reportedNumThreads < MIN_LOGICAL_CORES) ? MIN_LOGICAL_CORES : reportedNumThreads;
 }