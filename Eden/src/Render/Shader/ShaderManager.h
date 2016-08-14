#pragma once
#include "Render/Shader/ShaderTechnique.h"
#include "Core/Containers/DynamicArray.h"
#include <map>

class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

private:
	std::map<std::string, ShaderTechnique*> mShaderTechniqueLookup;
	DynamicArray<ShaderTechnique*> mShaderTechniques;
};