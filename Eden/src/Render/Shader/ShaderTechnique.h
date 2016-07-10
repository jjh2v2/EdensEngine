#include "Render/Shader/ShaderPipelineDefinition.h"
#include "Core/Containers/DynamicArray.h"
#include <map>

enum class ShaderTechniqueFlag : uint64
{
	NORMAL_MAP = 0,
	ROUGHMETAL_MAP = 1
};

class ShaderTechnique
{
public:
	ShaderTechnique();
	~ShaderTechnique();

	void AddAndCompilePermutation(uint64 flags);

private:
	std::map<uint64, ShaderPipelineDefinition*> mShaderPipelineMap;
};