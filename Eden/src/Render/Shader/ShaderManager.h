#pragma once
#include "Render/Shader/Shader.h"
#include "Core/Containers/DynamicArray.h"
#include <map>

class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

private:
	std::map<std::string, Shader*> mShaderLookup;
	DynamicArray<Shader*> mShaders;
};