#include "Render/RayTrace/RayTraceShaderManager.h"
#include "Render/DirectX/DXR/D3D12RaytracingHelpers.hpp"
#include "Render/DirectX/Direct3DManager.h"
#include "Render/Shader/RootSignature/RootSignatureManager.h"
#include "Util/String/StringConverter.h"
#include <fstream>

RayTraceShaderManager::RayTraceShaderManager(Direct3DManager *direct3DManager, RootSignatureManager *rootSignatureManager)
{
    mDirect3DManager = direct3DManager;
    mRootSignatureManager = rootSignatureManager;

    Direct3DUtils::ThrowIfHRESULTFailed(mDXC.DxcSupport.Initialize());
    Direct3DUtils::ThrowIfHRESULTFailed(mDXC.DxcSupport.CreateInstance(CLSID_DxcCompiler, &mDXC.DxcCompiler));
    Direct3DUtils::ThrowIfHRESULTFailed(mDXC.DxcSupport.CreateInstance(CLSID_DxcLibrary, &mDXC.DxcLibrary));
    Direct3DUtils::ThrowIfHRESULTFailed(mDXC.DxcLibrary->CreateIncludeHandler(&mDXC.DxcIncludeHandler));

    LoadAllPipelines();
}

RayTraceShaderManager::~RayTraceShaderManager()
{
    const uint32 numPipelines = mRayTracePipelines.CurrentSize();
    Direct3DContextManager *contextManager = mDirect3DManager->GetContextManager();

    for (uint32 i = 0; i < numPipelines; i++)
    {
        RayTracePSO *currentPipeline = mRayTracePipelines[i];
        currentPipeline->GlobalRootSignature = NULL;
        currentPipeline->RayTraceStateObjectProperties->Release();
        currentPipeline->RayTraceStateObject->Release();
        contextManager->FreeRayTraceBuffer(currentPipeline->GenShaderTable);
        contextManager->FreeRayTraceBuffer(currentPipeline->MissShaderTable);
        contextManager->FreeRayTraceBuffer(currentPipeline->HitGroupShaderTable);
        currentPipeline->GenShader->Release();
        currentPipeline->MissShader->Release();
        currentPipeline->ClosestHitShader->Release();

        delete currentPipeline;
    }

    mRayTracePipelineLookup.clear();
    mRayTracePipelines.Clear();

    mDXC.DxcIncludeHandler->Release();
    mDXC.DxcLibrary->Release();
    mDXC.DxcCompiler->Release();
    mDXC.DxcSupport.Cleanup();
}

RayTraceShaderManager::RayTracePSO *RayTraceShaderManager::GetRayTracePipeline(const std::string &pipelineName)
{
    if (mRayTracePipelineLookup.find(pipelineName) == mRayTracePipelineLookup.end())
    {
        Application::Assert(false);
        return NULL;
    }

    return mRayTracePipelineLookup[pipelineName];
}

void RayTraceShaderManager::LoadAllPipelines()
{
    mManifestLoader.LoadManifest(ApplicationSpecification::RayShaderManifestFileLocation);

    DynamicArray<std::string, false> &fileNames = mManifestLoader.GetFileNames();
    for (uint32 i = 0; i < fileNames.CurrentSize(); i++)
    {
        size_t lastSlash = fileNames[i].find_last_of("/");
        size_t lastDot = fileNames[i].find_last_of(".");
        std::string justFileName = fileNames[i].substr(lastSlash + 1, (lastDot - lastSlash) - 1);

        RayTracePSO *rayTracePipeline = LoadPipeline(fileNames[i].c_str());

        mRayTracePipelineLookup.insert(std::pair<std::string, RayTracePSO*>(justFileName, rayTracePipeline));
        mRayTracePipelines.Add(rayTracePipeline);
    }
}

RayTraceShaderManager::RayTracePSO *RayTraceShaderManager::LoadPipeline(const char *fileName)
{
    std::ifstream file(fileName);
    std::string line;
    char *removeChars = "[]";
    std::string shaderLocation = "../Eden/data/HLSL/RayTrace/";

    WCHAR genShaderFilePath[256] = {};
    WCHAR genShaderEntryPoint[256] = {};
    WCHAR missShaderFilePath[256] = {};
    WCHAR missShaderEntryPoint[256] = {};
    WCHAR closestHitShaderFilePath[256] = {};
    WCHAR closestHitShaderEntryPoint[256] = {};
    WCHAR *hitGroupExport = L"HitGroup";
    RootSignatureType genShaderLocalRootSignature = RootSignatureType_Ray_Empty_Local;
    RootSignatureType missShaderLocalRootSignature = RootSignatureType_Ray_Empty_Local;
    RootSignatureType hitGroupShaderLocalRootSignature = RootSignatureType_Ray_Empty_Local;
    RootSignatureType globalRootSignature = RootSignatureType_Ray_Barycentric_Global;

    uint32 payLoadFloatCount = 0;

    if (file.is_open())
    {
        std::getline(file, line);

        if (strcmp(line.c_str(), "GenShader") == 0)
        {
            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            line.insert(0, shaderLocation);
            StringConverter::StringToWCHAR(line, genShaderFilePath, 256);

            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            StringConverter::StringToWCHAR(line, genShaderEntryPoint, 256);

            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            genShaderLocalRootSignature = (RootSignatureType)StringConverter::StringToUint(line);

            std::getline(file, line);
        }

        if (strcmp(line.c_str(), "MissShader") == 0)
        {
            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            line.insert(0, shaderLocation);
            StringConverter::StringToWCHAR(line, missShaderFilePath, 256);

            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            StringConverter::StringToWCHAR(line, missShaderEntryPoint, 256);

            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            missShaderLocalRootSignature = (RootSignatureType)StringConverter::StringToUint(line);

            std::getline(file, line);
        }

        if (strcmp(line.c_str(), "ClosestHitShader") == 0)
        {
            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            line.insert(0, shaderLocation);
            StringConverter::StringToWCHAR(line, closestHitShaderFilePath, 256);

            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            StringConverter::StringToWCHAR(line, closestHitShaderEntryPoint, 256);

            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            hitGroupShaderLocalRootSignature = (RootSignatureType)StringConverter::StringToUint(line);

            std::getline(file, line);
        }

        if (strcmp(line.c_str(), "GlobalRootSignature") == 0)
        {
            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            uint32 rootSignatureID = StringConverter::StringToUint(line);
            globalRootSignature = (RootSignatureType)rootSignatureID;

            std::getline(file, line);
        }

        if (strcmp(line.c_str(), "Payload") == 0)
        {
            std::getline(file, line);
            StringConverter::RemoveCharsFromString(line, removeChars);
            payLoadFloatCount = StringConverter::StringToUint(line);
        }
    }

    file.close();

    RayTracePSO *newRayTracePipeline = new RayTracePSO();
    newRayTracePipeline->GenShader = CompileRayShader(genShaderFilePath);
    newRayTracePipeline->MissShader = CompileRayShader(missShaderFilePath);
    newRayTracePipeline->ClosestHitShader = CompileRayShader(closestHitShaderFilePath);
    newRayTracePipeline->GlobalRootSignature = mRootSignatureManager->GetRootSignature(globalRootSignature).RootSignature;

    CD3D12_STATE_OBJECT_DESC raytracingPipelineDesc{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    CD3D12_DXIL_LIBRARY_SUBOBJECT *genLibrarySubObject = raytracingPipelineDesc.CreateSubobject<CD3D12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE genLibraryByteCode = CD3DX12_SHADER_BYTECODE(newRayTracePipeline->GenShader->GetBufferPointer(), newRayTracePipeline->GenShader->GetBufferSize());
    genLibrarySubObject->SetDXILLibrary(&genLibraryByteCode);
    genLibrarySubObject->DefineExport(genShaderEntryPoint);

    CD3D12_DXIL_LIBRARY_SUBOBJECT *missLibrarySubObject = raytracingPipelineDesc.CreateSubobject<CD3D12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE missLibraryByteCode = CD3DX12_SHADER_BYTECODE(newRayTracePipeline->MissShader->GetBufferPointer(), newRayTracePipeline->MissShader->GetBufferSize());
    missLibrarySubObject->SetDXILLibrary(&missLibraryByteCode);
    missLibrarySubObject->DefineExport(missShaderEntryPoint);

    CD3D12_DXIL_LIBRARY_SUBOBJECT *closestHitLibrarySubObject = raytracingPipelineDesc.CreateSubobject<CD3D12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE closestHitLibraryByteCode = CD3DX12_SHADER_BYTECODE(newRayTracePipeline->ClosestHitShader->GetBufferPointer(), newRayTracePipeline->ClosestHitShader->GetBufferSize());
    closestHitLibrarySubObject->SetDXILLibrary(&closestHitLibraryByteCode);
    closestHitLibrarySubObject->DefineExport(closestHitShaderEntryPoint);

    CD3D12_HIT_GROUP_SUBOBJECT *hitGroupSubObject = raytracingPipelineDesc.CreateSubobject<CD3D12_HIT_GROUP_SUBOBJECT>();
    hitGroupSubObject->SetClosestHitShaderImport(closestHitShaderEntryPoint);
    hitGroupSubObject->SetHitGroupExport(hitGroupExport);
    hitGroupSubObject->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    CD3D12_RAYTRACING_SHADER_CONFIG_SUBOBJECT *shaderConfig = raytracingPipelineDesc.CreateSubobject<CD3D12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    UINT payloadSize = payLoadFloatCount * sizeof(float);   // float4 payload
    UINT attributeSize = 2 * sizeof(float); // float2 barycentrics
    shaderConfig->Config(payloadSize, attributeSize);

    CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT *genLocalRootSignature = raytracingPipelineDesc.CreateSubobject<CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    genLocalRootSignature->SetRootSignature(mRootSignatureManager->GetRootSignature(genShaderLocalRootSignature).RootSignature);

    CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT *genRootSignatureAssociation = raytracingPipelineDesc.CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    genRootSignatureAssociation->SetSubobjectToAssociate(*genLocalRootSignature);
    genRootSignatureAssociation->AddExport(genShaderEntryPoint);

    CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT *missLocalRootSignature = raytracingPipelineDesc.CreateSubobject<CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    missLocalRootSignature->SetRootSignature(mRootSignatureManager->GetRootSignature(missShaderLocalRootSignature).RootSignature);

    CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT *missRootSignatureAssociation = raytracingPipelineDesc.CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    missRootSignatureAssociation->SetSubobjectToAssociate(*missLocalRootSignature);
    missRootSignatureAssociation->AddExport(missShaderEntryPoint);
    
    CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT *hitGroupLocalRootSignature = raytracingPipelineDesc.CreateSubobject<CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    hitGroupLocalRootSignature->SetRootSignature(mRootSignatureManager->GetRootSignature(hitGroupShaderLocalRootSignature).RootSignature);

    CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT *hitGroupRootSignatureAssociation = raytracingPipelineDesc.CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    hitGroupRootSignatureAssociation->SetSubobjectToAssociate(*hitGroupLocalRootSignature);
    hitGroupRootSignatureAssociation->AddExport(hitGroupExport);

    CD3D12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT *globalRootSignatureObject = raytracingPipelineDesc.CreateSubobject<CD3D12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignatureObject->SetRootSignature(newRayTracePipeline->GlobalRootSignature);

    CD3D12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT *pipelineConfig = raytracingPipelineDesc.CreateSubobject<CD3D12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    UINT maxRecursionDepth = 1;                 // 1 = primary rays only. 
    pipelineConfig->Config(maxRecursionDepth);

    Direct3DUtils::ThrowIfHRESULTFailed(mDirect3DManager->GetRayTraceDevice()->CreateStateObject(raytracingPipelineDesc, IID_PPV_ARGS(&newRayTracePipeline->RayTraceStateObject)));
    Direct3DUtils::ThrowIfHRESULTFailed(newRayTracePipeline->RayTraceStateObject->QueryInterface(IID_PPV_ARGS(&newRayTracePipeline->RayTraceStateObjectProperties)));

    uint32 shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    uint32 shaderRecordSize = MathHelper::AlignU32(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    uint8_t *mappedData = NULL;

    newRayTracePipeline->GenShaderTable = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(shaderRecordSize, D3D12_RESOURCE_STATE_GENERIC_READ, RayTraceBuffer::RayTraceBufferType_Shader_Binding_Table_Storage);
    const void* genShaderId = newRayTracePipeline->RayTraceStateObjectProperties->GetShaderIdentifier(genShaderEntryPoint);

    newRayTracePipeline->GenShaderTable->GetResource()->Map(0, NULL, reinterpret_cast<void**>(&mappedData));
    memcpy(mappedData, genShaderId, shaderIdentifierSize);
    newRayTracePipeline->GenShaderTable->GetResource()->Unmap(0, NULL);

    newRayTracePipeline->MissShaderTable = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(shaderRecordSize, D3D12_RESOURCE_STATE_GENERIC_READ, RayTraceBuffer::RayTraceBufferType_Shader_Binding_Table_Storage);
    const void* missShaderId = newRayTracePipeline->RayTraceStateObjectProperties->GetShaderIdentifier(missShaderEntryPoint);

    newRayTracePipeline->MissShaderTable->GetResource()->Map(0, NULL, reinterpret_cast<void**>(&mappedData));
    memcpy(mappedData, missShaderId, shaderIdentifierSize);
    newRayTracePipeline->MissShaderTable->GetResource()->Unmap(0, NULL);

    newRayTracePipeline->HitGroupShaderTable = mDirect3DManager->GetContextManager()->CreateRayTraceBuffer(shaderRecordSize, D3D12_RESOURCE_STATE_GENERIC_READ, RayTraceBuffer::RayTraceBufferType_Shader_Binding_Table_Storage);
    const void* hitGroupShaderId = newRayTracePipeline->RayTraceStateObjectProperties->GetShaderIdentifier(hitGroupExport);

    newRayTracePipeline->HitGroupShaderTable->GetResource()->Map(0, NULL, reinterpret_cast<void**>(&mappedData));
    memcpy(mappedData, hitGroupShaderId, shaderIdentifierSize);
    newRayTracePipeline->HitGroupShaderTable->GetResource()->Unmap(0, NULL);

    return newRayTracePipeline;
}

IDxcBlob *RayTraceShaderManager::CompileRayShader(WCHAR *fileName)
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
    Direct3DUtils::ThrowIfHRESULTFailed(mDXC.DxcLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)rayShaderString.c_str(), (uint32_t)rayShaderString.size(), 0, &rayShaderBlobEncoding));

    IDxcOperationResult* compilationResult;
    Direct3DUtils::ThrowIfHRESULTFailed(mDXC.DxcCompiler->Compile(rayShaderBlobEncoding, fileName, L"", L"lib_6_3", nullptr, 0, nullptr, 0,
        mDXC.DxcIncludeHandler, &compilationResult));

    HRESULT resultCode;
    Direct3DUtils::ThrowIfHRESULTFailed(compilationResult->GetStatus(&resultCode));
    if (FAILED(resultCode))
    {
        IDxcBlobEncoding* pError;
        if (FAILED(compilationResult->GetErrorBuffer(&pError)))
        {
            Direct3DUtils::ThrowLogicError("Failed to get shader compiler error.");
        }

        const char *pStart = pError ? (const char *)pError->GetBufferPointer() : NULL;
        Direct3DUtils::ThrowLogicError("Shader compilation error.");
    }

    IDxcBlob* shaderBlob;
    Direct3DUtils::ThrowIfHRESULTFailed(compilationResult->GetResult(&shaderBlob));

    return shaderBlob;
}