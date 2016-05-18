#include "Core/Platform/PlatformCore.h"

bool ApplicationSpecification::ForceAllTexturesToSRGB = false;
char *ApplicationSpecification::TextureManifestFileLocation = "../Eden/data/Manifests/TextureManifest.emf";

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
