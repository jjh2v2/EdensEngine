#include "Render/Shader/ShaderManager.h"
#include "Util/String/StringConverter.h"
#include "Render/Shader/State/ShaderPipelineStateCreator.h"

ShaderManager::ShaderManager(ID3D12Device *device)
{
	mDevice = device;

	ShaderPipelineStateCreator::BuildPipelineStates();
	mRootSignatureManager = new RootSignatureManager(mDevice);
	
    LoadAllShaders(mDevice);
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

    ShaderPipelineStateCreator::DestroyPipelineStates();
}

ShaderPSO *ShaderManager::GetShader(const std::string &shaderName, const ShaderPipelinePermutation &permutation)
{ 
    if (mShaderTechniqueLookup.find(shaderName) == mShaderTechniqueLookup.end())
    {
        Application::Assert(false);
        return NULL;
    }

	ShaderTechniqueInfo techniqueInfo = mShaderTechniqueLookup[shaderName];

	if (techniqueInfo.Technique->HasShader(permutation))
	{
		return techniqueInfo.Technique->GetShader(permutation);
	}
	else
	{
		techniqueInfo.Technique->AddAndCompilePermutation(mDevice, permutation, mRootSignatureManager->GetRootSignature(techniqueInfo.SignatureType).RootSignature);
		return techniqueInfo.Technique->GetShader(permutation);
	}
}

void ShaderManager::LoadAllShaders(ID3D12Device *device)
{
	mManifestLoader.LoadManifest(ApplicationSpecification::ShaderManifestFileLocation);

	DynamicArray<std::string, false> &fileNames = mManifestLoader.GetFileNames();
	for (uint32 i = 0; i < fileNames.CurrentSize(); i++)
	{
		size_t lastSlash = fileNames[i].find_last_of("/");
		size_t lastDot = fileNames[i].find_last_of(".");
		std::string justFileName = fileNames[i].substr(lastSlash + 1, (lastDot - lastSlash) - 1);

		ShaderTechniqueInfo shaderTechniqueInfo = LoadShader(device, fileNames[i].c_str());

		mShaderTechniqueLookup.insert(std::pair<std::string, ShaderTechniqueInfo>(justFileName, shaderTechniqueInfo));
		mShaderTechniques.Add(shaderTechniqueInfo.Technique);
	}

	PreloadExpectedPermutations();
}

void ShaderManager::PreloadExpectedPermutations()
{
	//TDA: Fill this in. Keep a list of shaders that should be preloaded with their permutations so that it's not on-demand.
}

ShaderManager::ShaderTechniqueInfo ShaderManager::LoadShader(ID3D12Device *device, const char *fileName)
{
	ShaderTechniqueInfo techniqueInfo;

	ShaderPipelineDefinition pipelineDefinition;
	pipelineDefinition.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	std::ifstream file(fileName);
	std::string line;
	char *removeChars = "[]";
	std::string shaderLocation = "../Eden/data/HLSL/";

	if (file.is_open())
	{
		std::getline(file, line);

        if (strcmp(line.c_str(), "ComputeShader") == 0)
        {
            pipelineDefinition.IsRenderShader = false;

            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            line.insert(0, shaderLocation);
            StringConverter::StringToWCHAR(line, pipelineDefinition.CSFilePath, 256);

            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            pipelineDefinition.CSEntry = line;

            std::getline(file, line);
        }
        else
        {
            pipelineDefinition.IsRenderShader = true;

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
                pipelineDefinition.HasTessellation = true;

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
                pipelineDefinition.HasTessellation = true;

                std::getline(file, line);
                StringConverter::RemoveCharsFromString(line, removeChars);
                line.insert(0, shaderLocation);
                StringConverter::StringToWCHAR(line, pipelineDefinition.DSFilePath, 256);

                std::getline(file, line);
                StringConverter::RemoveCharsFromString(line, removeChars);
                pipelineDefinition.DSEntry = line;

                std::getline(file, line);
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

                std::getline(file, line);
            }
        }

        if (strcmp(line.c_str(), "RootSignature") == 0)
        {
            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            uint32 rootSignatureID = StringConverter::StringToUint(line);
            techniqueInfo.SignatureType = (RootSignatureType)rootSignatureID;
        }
	}

	file.close();

	std::string fileNameString = fileName;
	uint32 lastSlash = (uint32)fileNameString.find_last_of("/");
	uint32 lastDot = (uint32)fileNameString.find_last_of(".");
	pipelineDefinition.ShaderOutputName = fileNameString.substr(lastSlash + 1, (lastDot - lastSlash) - 1);

	techniqueInfo.Technique = new ShaderTechnique(device, pipelineDefinition);
	
	return techniqueInfo;
}