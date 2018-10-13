#include "Render/Mesh/MeshManager.h"
#include "Util/File/FileUtil.h"
#include "Util/String/StringConverter.h"
#include <assimp/Importer.hpp>      
#include <assimp/scene.h>           
#include <assimp/postprocess.h> 

MeshManager::MeshManager()
{
}

MeshManager::~MeshManager()
{
	for (uint32 i = 0; i < mMeshes.CurrentSize(); i++)
	{
		delete mMeshes[i];
		mMeshes[i] = NULL;
	}
	mMeshes.Clear();
	mMeshLookup.clear();
}

Mesh *MeshManager::GetMesh(const std::string &meshName)
{
	return mMeshLookup[meshName];
}

void MeshManager::LoadAllMeshes(Direct3DManager *direct3DManager)
{
	mManifestLoader.LoadManifest(ApplicationSpecification::MeshManifestFileLocation);

	std::string serializedFileLocation = ApplicationSpecification::MeshSerializationLocation;

	DynamicArray<std::string, false> &fileNames = mManifestLoader.GetFileNames();

	for(uint32 i = 0; i < fileNames.CurrentSize(); i++)
	{
		uint32 lastSlash = (uint32)fileNames[i].find_last_of("/");
		uint32 lastDot = (uint32)fileNames[i].find_last_of(".");
		std::string justFileName = fileNames[i].substr(lastSlash+1, (lastDot-lastSlash)-1);
		Mesh *newMesh = NULL;

		std::string serializedName = serializedFileLocation + justFileName + ".sem";

		if(StringConverter::DoesStringEndWith(fileNames[i], ".fbx"))
		{
			if(FileUtil::DoesFileExist(serializedName) && !ApplicationSpecification::RebuildAllMeshes)
			{
				newMesh = DeserializeMeshFromFile(direct3DManager, (char*)serializedName.c_str());
			}
			else
			{
				newMesh = LoadFBXMesh(direct3DManager, (char*)fileNames[i].c_str(), (char*)serializedName.c_str());
			}
		}
		else
		{
			if(FileUtil::DoesFileExist(serializedName) && !ApplicationSpecification::RebuildAllMeshes)
			{
				newMesh = DeserializeMeshFromFile(direct3DManager, (char*)serializedName.c_str());
			}
			else
			{
				newMesh = LoadFromAssimp(direct3DManager, (char*)fileNames[i].c_str(), (char*)serializedName.c_str());
			}
		}

		mMeshLookup.insert(std::pair<std::string, Mesh*>(justFileName, newMesh));
		mMeshes.Add(newMesh);
	}
}

Mesh *MeshManager::LoadFBXMesh(Direct3DManager *direct3DManager, char *fileName, char *serializationFile)
{
	DynamicArray<MeshVertex> fbxData;
	DynamicArray<uint32> indexSplits;

	mFBXLoader.LoadFBX(fileName, fbxData, indexSplits);

	uint32 fbxDataCount = fbxData.CurrentSize();

	uint32 *meshIndices = new uint32[fbxDataCount];
	for (uint32 i = 0; i < fbxDataCount; i++)
	{
		meshIndices[i] = i;
	}

	Mesh *fbxMesh = new Mesh(direct3DManager, fbxDataCount, fbxDataCount, fbxData.GetInnerArrayCopy(), indexSplits, meshIndices);
	SerializeMeshToFile(fbxMesh, serializationFile);

	return fbxMesh;
}

void MeshManager::SerializeMeshToFile(Mesh *mesh, char *fileName)
{
	std::ofstream outputFile(fileName, std::ios::binary);

	MeshVertex *meshData = mesh->GetMeshData();
	uint32 indexCount = mesh->GetIndexCount();

	outputFile.write((char*)&indexCount, sizeof(indexCount));
	outputFile.write((char*)meshData, sizeof(MeshVertex) * mesh->GetIndexCount());
	outputFile.write((char*)mesh->GetIndices(), sizeof(uint32) * mesh->GetIndexCount());
	uint32 splitCount = mesh->GetMeshSplitCount();

	outputFile.write((char*)&splitCount, sizeof(splitCount));
	for(uint32 i = 0; i < splitCount; i++)
	{
		uint32 indexSplit = mesh->GetMeshIndexSplitByIndex(i);
		outputFile.write((char*)&indexSplit, sizeof(indexSplit));
	}

	outputFile.close();
}

Mesh *MeshManager::DeserializeMeshFromFile(Direct3DManager *direct3DManager, char *fileName)
{
	uint32 indexCount = 0;
	std::ifstream inputFile(fileName, std::ios::binary);

	inputFile.read((char *)&indexCount, sizeof(indexCount));

	MeshVertex *meshData = new MeshVertex[indexCount];
	uint32 *meshIndices = new uint32[indexCount];

	inputFile.read((char *)meshData, sizeof(MeshVertex) * indexCount);
	inputFile.read((char *)meshIndices, sizeof(uint32) * indexCount);

	uint32 indexSplitCount = 0;
	inputFile.read((char *)&indexSplitCount, sizeof(indexSplitCount));

	DynamicArray<uint32> indexSplits(indexSplitCount);

	for(uint32 i = 0; i < indexSplitCount; i++)
	{
		uint32 indexNum = 0;
		inputFile.read((char *)&indexNum, sizeof(indexNum));
		indexSplits.Add(indexNum);
	}

	inputFile.close();

	Mesh *newMesh = new Mesh(direct3DManager, indexCount, indexCount, meshData, indexSplits, meshIndices);

	return newMesh;
}

Mesh *MeshManager::LoadFromAssimp(Direct3DManager *direct3DManager, char *fileName, char *serializationFile)
{
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(fileName,
		aiProcess_Triangulate            |
		aiProcess_CalcTangentSpace		 | 
		aiProcess_OptimizeMeshes 
		);

	if( !scene)
	{
		return NULL;
	}

	uint32 vertCount = 0;
	uint32 indexCount = 0;
	DynamicArray<uint32> indexSplits;

	for(uint32 i = 0; i < scene->mNumMeshes; i++)
	{
		vertCount += scene->mMeshes[i]->mNumVertices;
		indexCount += scene->mMeshes[i]->mNumFaces * 3;
		indexSplits.Add(indexCount);
	}

	MeshVertex *meshData = new MeshVertex[vertCount];
	uint32 *indices = new uint32[indexCount];

	uint32 meshDataIndex = 0;
	uint32 indexIndex = 0;

	for(uint32 i = 0; i < scene->mNumMeshes; i++)
	{
		for(uint32 f = 0; f < scene->mMeshes[i]->mNumFaces; f++)
		{
			indices[indexIndex] = scene->mMeshes[i]->mFaces[f].mIndices[0] + meshDataIndex;
			indexIndex++;
			indices[indexIndex] = scene->mMeshes[i]->mFaces[f].mIndices[1] + meshDataIndex;
			indexIndex++;
			indices[indexIndex] = scene->mMeshes[i]->mFaces[f].mIndices[2] + meshDataIndex;
			indexIndex++;
		}

		for(uint32 j = 0; j < scene->mMeshes[i]->mNumVertices; j++)
		{
			aiVector3D vert = scene->mMeshes[i]->mVertices[j];
			aiVector3D normal = scene->mMeshes[i]->mNormals[j];
			aiVector3D texcoord = scene->mMeshes[i]->mTextureCoords[0][j];
			aiVector3D tangent = scene->mMeshes[i]->mTangents[j];
			aiVector3D bitangent = scene->mMeshes[i]->mBitangents[j];

			meshData[meshDataIndex].Position = Vector4(vert.x, vert.y, vert.z, 1.0f);
			meshData[meshDataIndex].Normal = Vector3(normal.x, normal.y, normal.z);
			meshData[meshDataIndex].TexCoord = Vector4(texcoord.x, 1.0f - texcoord.y, texcoord.x, 1.0f - texcoord.y);
			meshData[meshDataIndex].Tangent = Vector3(tangent.x, tangent.y, tangent.z);
			meshData[meshDataIndex].Binormal = Vector3(bitangent.x, bitangent.y, bitangent.z);

			meshDataIndex++;
		}
	}

	Mesh *mesh = new Mesh(direct3DManager, vertCount, indexCount, meshData, indexSplits, indices);

	SerializeMeshToFile(mesh, serializationFile);

	return mesh;
}