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

ShaderPSO *ShaderManager::GetShader(std::string shaderName, const ShaderPipelinePermutation &permutation)
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
    LoadRayTraceShaders();
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

void ShaderManager::LoadRayTraceShaders()
{
    //TDA: clean up memory use
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcSupport.Initialize());
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcSupport.CreateInstance(CLSID_DxcCompiler, &mRayTraceShaderBuilder.DxcCompiler));
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcSupport.CreateInstance(CLSID_DxcLibrary, &mRayTraceShaderBuilder.DxcLibrary));
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcLibrary->CreateIncludeHandler(&mRayTraceShaderBuilder.DxcIncludeHandler));

    mRayGenerationShader = CompileRayShader(L"../Eden/data/HLSL/RayTrace/RayTestGen.hlsl");
    mRayClosestHitShader = CompileRayShader(L"../Eden/data/HLSL/RayTrace/RayTestClosestHit.hlsl");
    mRayMissShader = CompileRayShader(L"../Eden/data/HLSL/RayTrace/RayTestMiss.hlsl");
}

IDxcBlob *ShaderManager::CompileRayShader(WCHAR *fileName)
{
    std::ifstream rayShaderFile(fileName);
    if (!rayShaderFile.is_open())
    {
        Application::Assert(false);
    }

    std::stringstream strStream;
    strStream << rayShaderFile.rdbuf();
    std::string rayShaderString = strStream.str();

    IDxcBlobEncoding *rayShaderBlobEncoding;
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)rayShaderString.c_str(), (uint32_t)rayShaderString.size(), 0, &rayShaderBlobEncoding));

    IDxcOperationResult* compilationResult;
    Direct3DUtils::ThrowIfHRESULTFailed(mRayTraceShaderBuilder.DxcCompiler->Compile(rayShaderBlobEncoding, fileName, L"", L"lib_6_3", nullptr, 0, nullptr, 0,
        mRayTraceShaderBuilder.DxcIncludeHandler, &compilationResult));

    HRESULT resultCode;
    Direct3DUtils::ThrowIfHRESULTFailed(compilationResult->GetStatus(&resultCode));
    if (FAILED(resultCode))
    {
        IDxcBlobEncoding* pError;
        if (FAILED(compilationResult->GetErrorBuffer(&pError)))
        {
            Direct3DUtils::ThrowLogicError("Failed to get shader compiler error.");
        }

        Direct3DUtils::ThrowLogicError("Shader compilation error.");
    }

    IDxcBlob* shaderBlob;
    Direct3DUtils::ThrowIfHRESULTFailed(compilationResult->GetResult(&shaderBlob));
    return shaderBlob;
}

IDxcBlob* ShaderManager::CompileShaderLibrary(LPCWSTR fileName)
{
    static dxc::DxcDllSupport gDxcDllHelper;
    static IDxcCompiler* pCompiler = nullptr;
    static IDxcLibrary* pLibrary = nullptr;
    static IDxcIncludeHandler* dxcIncludeHandler;

    HRESULT hr;

    Direct3DUtils::ThrowIfHRESULTFailed(gDxcDllHelper.Initialize());

    // Initialize the DXC compiler and compiler helper
    if (!pCompiler)
    {
        Direct3DUtils::ThrowIfHRESULTFailed(gDxcDllHelper.CreateInstance(CLSID_DxcCompiler, &pCompiler));
        Direct3DUtils::ThrowIfHRESULTFailed(gDxcDllHelper.CreateInstance(CLSID_DxcLibrary, &pLibrary));
        Direct3DUtils::ThrowIfHRESULTFailed(pLibrary->CreateIncludeHandler(&dxcIncludeHandler));
    }
    // Open and read the file
    std::ifstream shaderFile(fileName);
    if (shaderFile.good() == false)
    {
        throw std::logic_error("Cannot find shader file");
    }
    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    std::string sShader = strStream.str();

    // Create blob from the string
    IDxcBlobEncoding* pTextBlob;
    Direct3DUtils::ThrowIfHRESULTFailed(pLibrary->CreateBlobWithEncodingFromPinned(
        (LPBYTE)sShader.c_str(), (uint32_t)sShader.size(), 0, &pTextBlob));

    // Compile
    IDxcOperationResult* pResult;
    Direct3DUtils::ThrowIfHRESULTFailed(pCompiler->Compile(pTextBlob, fileName, L"", L"lib_6_3", nullptr, 0, nullptr, 0,
        dxcIncludeHandler, &pResult));

    // Verify the result
    HRESULT resultCode;
    Direct3DUtils::ThrowIfHRESULTFailed(pResult->GetStatus(&resultCode));
    if (FAILED(resultCode))
    {
        IDxcBlobEncoding* pError;
        hr = pResult->GetErrorBuffer(&pError);
        if (FAILED(hr))
        {
            throw std::logic_error("Failed to get shader compiler error");
        }

        // Convert error blob to a string
        std::vector<char> infoLog(pError->GetBufferSize() + 1);
        memcpy(infoLog.data(), pError->GetBufferPointer(), pError->GetBufferSize());
        infoLog[pError->GetBufferSize()] = 0;

        std::string errorMsg = "Shader Compiler Error:\n";
        errorMsg.append(infoLog.data());

        MessageBoxA(nullptr, errorMsg.c_str(), "Error!", MB_OK);
        throw std::logic_error("Failed compile shader");
    }

    IDxcBlob* pBlob;
    Direct3DUtils::ThrowIfHRESULTFailed(pResult->GetResult(&pBlob));
    return pBlob;
}