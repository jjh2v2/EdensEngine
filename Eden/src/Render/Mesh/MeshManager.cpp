#include "Render/Mesh/MeshManager.h"
#include "Util/File/FileUtil.h"
#include "Util/String/StringConverter.h"
#include <assimp/Importer.hpp>      
#include <assimp/scene.h>           
#include <assimp/postprocess.h>    

bool MeshManager::mRebuildAllMeshes = false;

MeshManager::MeshManager()
{
}

MeshManager::~MeshManager()
{
}

Mesh *MeshManager::GetMesh(std::string meshName)
{
	return mMeshLookup[meshName];
}

void MeshManager::LoadAllMeshes(Direct3DManager *direct3DManager)
{
	mManifestLoader.LoadManifest(ApplicationSpecification::MeshManifestFileLocation);

	std::string serializedFileLocation = "../EdensEngine/data/Meshes/Serialized/";

	DynamicArray<std::string> &fileNames = mManifestLoader.GetFileNames();

	for(uint32 i = 0; i < fileNames.CurrentSize(); i++)
	{
		uint32 lastSlash = (uint32)fileNames[i].find_last_of("/");
		uint32 lastDot = (uint32)fileNames[i].find_last_of(".");
		std::string justFileName = fileNames[i].substr(lastSlash+1, (lastDot-lastSlash)-1);
		Mesh *newMesh = NULL;

		std::string serializedName = serializedFileLocation + justFileName + ".sem";

		if(StringConverter::DoesStringEndWith(fileNames[i], ".fbx"))
		{
			if(FileUtil::DoesFileExist(serializedName) && !mRebuildAllMeshes)
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
			if(FileUtil::DoesFileExist(serializedName) && !mRebuildAllMeshes)
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
	DynamicArray<MeshVertexData> fbxData;
	DynamicArray<uint32> indexSplits;

	mFBXLoader.LoadFBX(fileName, fbxData, indexSplits);

	Mesh *fbxMesh = new Mesh(direct3DManager, fbxData.CurrentSize(), fbxData.CurrentSize(), fbxData.GetInnerArrayCopy(), indexSplits);
	SerializeMeshToFile(fbxMesh, serializationFile);

	return fbxMesh;
}

void MeshManager::SerializeMeshToFile(Mesh *mesh, char *fileName)
{
	std::ofstream outputFile(fileName, std::ios::binary);

	MeshVertexData *meshData = mesh->GetMeshData();
	uint32 indexCount = mesh->GetIndexCount();

	outputFile.write((char*)&indexCount, sizeof(indexCount));
	outputFile.write((char*)meshData, sizeof(MeshVertexData) * mesh->GetIndexCount());

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

	MeshVertexData *meshData = new MeshVertexData[indexCount];

	inputFile.read((char *)meshData, sizeof(MeshVertexData) * indexCount);

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

	Mesh *newMesh = new Mesh(direct3DManager, indexCount, indexCount, meshData, indexSplits);

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

	MeshVertexData *meshData = new MeshVertexData[vertCount];
	uint64 *indices = new uint64[indexCount];

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

			meshData[meshDataIndex].Position = Vector3(vert.x, vert.y, vert.z);
			meshData[meshDataIndex].Normal = Vector3(normal.x, normal.y, normal.z);
			meshData[meshDataIndex].TexCoord = Vector2(texcoord.x, 1.0f - texcoord.y);
			meshData[meshDataIndex].Tangent = Vector3(tangent.x, tangent.y, tangent.z);
			meshData[meshDataIndex].Binormal = Vector3(bitangent.x, bitangent.y, bitangent.z);

			meshDataIndex++;
		}
	}

	Mesh *mesh = new Mesh(direct3DManager, vertCount, indexCount, meshData, indexSplits, indices);

	SerializeMeshToFile(mesh, serializationFile);

	return mesh;
}

void MeshManager::ReleaseMeshes()
{
	for(uint32 i = 0; i < mMeshes.CurrentSize(); i++)
	{
		delete mMeshes[i];
		mMeshes[i] = NULL;
	}
	mMeshes.Clear();
	mMeshLookup.clear();
}