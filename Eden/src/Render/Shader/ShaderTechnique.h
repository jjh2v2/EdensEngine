#include "Render/Shader/Shader.h"
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
	ShaderTechnique(ShaderPipelineDefinition pipelineDefinition);
	~ShaderTechnique();

	void AddAndCompilePermutation(ShaderPipelinePermutation permutation);

private:
	std::map<ShaderPipelinePermutation, Shader*> mShaderPipelineMap;
	ShaderPipelineDefinition mBaseShaderPipeline;
};