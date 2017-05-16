#include "Render/Shader/ShaderManager.h"
#include "Util/String/StringConverter.h"
#include "Render/Shader/State/ShaderPipelineStateCreator.h"

ShaderManager::ShaderManager(ID3D12Device *device)
{
	ShaderPipelineStateCreator::BuildPipelineStates();
	mRootSignatureManager = new RootSignatureManager(device);
	LoadAllShaders(device);
}

ShaderManager::~ShaderManager()
{
	for (uint32 i = 0; i < mShaderTechniques.CurrentSize(); i++)
	{
		delete mShaderTechniques[i];
	}
	mShaderTechniques.Clear();
	mShaderTechniqueLookup.clear();

	delete mRootSignatureManager;
}

void ShaderManager::LoadAllShaders(ID3D12Device *device)
{
	mManifestLoader.LoadManifest(ApplicationSpecification::ShaderManifestFileLocation);

	//TDA: need to clean this up so I'm not manually entering permutations
	DynamicArray<std::string, false> &fileNames = mManifestLoader.GetFileNames();
	for (uint32 i = 0; i < fileNames.CurrentSize(); i++)
	{
		size_t lastSlash = fileNames[i].find_last_of("/");
		size_t lastDot = fileNames[i].find_last_of(".");
		std::string justFileName = fileNames[i].substr(lastSlash + 1, (lastDot - lastSlash) - 1);

		ShaderTechnique *shaderTechnique = LoadShader(device, fileNames[i].c_str());

		mShaderTechniqueLookup.insert(std::pair<std::string, ShaderTechnique*>(justFileName, shaderTechnique));
		mShaderTechniques.Add(shaderTechnique);
	}

	{
		ShaderPipelinePermutation permutation(Render_Standard, Target_GBuffer);
		mShaderTechniques[0]->AddAndCompilePermutation(device, permutation, mRootSignatureManager->GetRootSignature(RootSignatureType_GBuffer).RootSignature);
	}
	{
		ShaderPipelinePermutation permutation(Render_Standard, Target_Standard_BackBuffer);
		mShaderTechniques[1]->AddAndCompilePermutation(device, permutation, mRootSignatureManager->GetRootSignature(RootSignatureType_Simple_Color).RootSignature);
	}
	{
		ShaderPipelinePermutation permutation(Render_Standard, Target_Single_16);
		mShaderTechniques[1]->AddAndCompilePermutation(device, permutation, mRootSignatureManager->GetRootSignature(RootSignatureType_Simple_Color).RootSignature);
	}
	{
		ShaderPipelinePermutation permutation(Render_Standard_NoDepth, Target_Standard_BackBuffer_NoDepth);
		mShaderTechniques[2]->AddAndCompilePermutation(device, permutation, mRootSignatureManager->GetRootSignature(RootSignatureType_Simple_Copy).RootSignature);
	}
}

ShaderTechnique *ShaderManager::LoadShader(ID3D12Device *device, const char *fileName)
{
	ShaderPipelineDefinition pipelineDefinition;
	pipelineDefinition.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	std::ifstream file(fileName);
	std::string line;
	char *removeChars = "[]";
	std::string shaderLocation = "../Eden/data/HLSL/";

	if (file.is_open())
	{
		std::getline(file, line);

		if (strcmp(line.c_str(), "VertexShader") == 0)
		{
			std::getline(file, line);
			StringConverter::RemoveCharsFromString(line, removeChars);
			line.insert(0, shaderLocation);
			StringConverter::StringToWCHAR(line, pipelineDefinition.VSFilePath, 256);

			std::getline(file, line);
			StringConverter::RemoveCharsFromString(line, removeChars);
			pipelineDefinition.VSEntry = line;

			std::getline(file, line);
		}
		if (strcmp(line.c_str(), "HullShader") == 0)
		{
			pipelineDefinition.UsesTessellation = true;

			std::getline(file, line);
			StringConverter::RemoveCharsFromString(line, removeChars);
			line.insert(0, shaderLocation);
			StringConverter::StringToWCHAR(line, pipelineDefinition.HSFilePath, 256);

			std::getline(file, line);
			StringConverter::RemoveCharsFromString(line, removeChars);
			pipelineDefinition.HSEntry = line;

			std::getline(file, line);
		}
		if (strcmp(line.c_str(), "DomainShader") == 0)
		{
			pipelineDefinition.UsesTessellation = true;

			std::getline(file, line);
			StringConverter::RemoveCharsFromString(line, removeChars);
			line.insert(0, shaderLocation);
			StringConverter::StringToWCHAR(line, pipelineDefinition.DSFilePath, 256);

			std::getline(file, line);
			StringConverter::RemoveCharsFromString(line, removeChars);
			pipelineDefinition.DSEntry = line;
		}
		if (strcmp(line.c_str(), "PixelShader") == 0)
		{
			pipelineDefinition.HasPixelShader = true;
			
			std::getline(file, line);
			StringConverter::RemoveCharsFromString(line, removeChars);
			line.insert(0, shaderLocation);
			StringConverter::StringToWCHAR(line, pipelineDefinition.PSFilePath, 256);

			std::getline(file, line);
			StringConverter::RemoveCharsFromString(line, removeChars);
			pipelineDefinition.PSEntry = line;
		}
	}

	file.close();

	std::string fileNameString = fileName;
	uint32 lastSlash = (uint32)fileNameString.find_last_of("/");
	uint32 lastDot = (uint32)fileNameString.find_last_of(".");
	pipelineDefinition.ShaderOutputName = fileNameString.substr(lastSlash + 1, (lastDot - lastSlash) - 1);

	ShaderTechnique *newShaderTechnique = new ShaderTechnique(device, pipelineDefinition);
	return newShaderTechnique;
}