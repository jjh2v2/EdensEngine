#pragma once
#include "Core/Platform/PlatformCore.h"
#include <map>
#include <string>

enum class ShaderTechniqueFlag : uint64
{
	TECHNIQUE_NORMAL_MAP = 1 << 0,
	TECHNIQUE_ROUGHMETAL_MAP = 1 << 1
};

class ShaderDefineMapper
{
public:
	ShaderDefineMapper();
	~ShaderDefineMapper();

	static D3D_SHADER_MACRO *GetMarcosFromFlags(uint64 flags);

private:

	static std::map<ShaderTechniqueFlag, std::string> mShaderMacroMap;
	static std::map<uint64, D3D_SHADER_MACRO *> mCachedMacros;
};