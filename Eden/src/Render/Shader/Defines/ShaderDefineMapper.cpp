#include "Render/Shader/Defines/ShaderDefineMapper.h"
#include "Core/Containers/DynamicArray.h"

ShaderDefineMapper::ShaderDefineMapper()
{
	mShaderMacroMap.insert(std::pair<ShaderTechniqueFlag, std::string>(ShaderTechniqueFlag::TECHNIQUE_NORMAL_MAP, "USE_NORMAL_MAP"));
	mShaderMacroMap.insert(std::pair<ShaderTechniqueFlag, std::string>(ShaderTechniqueFlag::TECHNIQUE_ROUGHMETAL_MAP, "USE_ROUGHMETAL_MAP"));
}

ShaderDefineMapper::~ShaderDefineMapper()
{

}

D3D_SHADER_MACRO* ShaderDefineMapper::GetMarcosFromFlags(uint64 flags)
{
	if (flags == 0)
	{
		return NULL;
	}

	if (mCachedMacros.find(flags) != mCachedMacros.end())
	{
		return mCachedMacros[flags];
	}

	DynamicArray<ShaderTechniqueFlag> enabledFlags;

	for (uint32 i = 0; i < 64; i++)
	{
		uint64 comp = 1 << i;
		if (flags & comp > 0)
		{
			enabledFlags.Add((ShaderTechniqueFlag)comp);
		}
	}

	//finish this alex
}
