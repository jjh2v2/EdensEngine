#include "Render/Mesh/MeshManager.h"
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

void MeshManager::LoadAllMeshes(ID3D12Device* device)
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

		//if(StringUtil::DoesStringEndWith(fileNames[i], ".fbx"))
		//{
		//	if(FileUtil::DoesFileExist(serializedName) && !mRebuildAllMeshes)
		//	{
		//		newMesh = DeserializeMeshFromFile(device, (char*)serializedName.c_str());
		//	}
		//	else
		//	{
				newMesh = LoadFBXMesh(device, (char*)fileNames[i].c_str(), (char*)serializedName.c_str());
		//	}
		//}
		//else
		//{
		//	if(FileUtil::DoesFileExist(serializedName) && !mRebuildAllMeshes)
		//	{
		//		newMesh = DeserializeMeshFromFile(device, (char*)serializedName.c_str());
		//	}
		//	else
		//	{
		//		newMesh = LoadFromAssimp(device, (char*)fileNames[i].c_str(), (char*)serializedName.c_str());
		//	}
		//}

		mMeshLookup.insert(std::pair<std::string, Mesh*>(justFileName, newMesh));
		mMeshes.Add(newMesh);
	}
}

Mesh *MeshManager::LoadFBXMesh(ID3D12Device* device, char *fileName, char *serializationFile)
{
	DynamicArray<MeshVertexData> fbxData;
	DynamicArray<int> indexSplits;

	mFBXLoader.LoadFBX(fileName, fbxData, indexSplits);

	Mesh *fbxMesh = new Mesh();
	fbxMesh->InitializeWithMeshInfo(device, fbxData.CurrentSize(), fbxData.CurrentSize(), fbxData.GetInnerArrayCopy(), indexSplits);
	
	SerializeMeshToFile(fbxMesh, serializationFile);

	return fbxMesh;
}

void MeshManager::SerializeMeshToFile(Mesh *mesh, char* fileName)
{
	std::ofstream outputFile(fileName, std::ios::binary);

	MeshVertexData *meshData = mesh->GetMeshData();
	int indexCount = mesh->GetIndexCount();

	outputFile.write((char*)&indexCount, sizeof(indexCount));
	outputFile.write((char*)meshData, sizeof(MeshVertexData) * mesh->GetIndexCount());

	int splitCount = mesh->GetMeshSplitCount();

	outputFile.write((char*)&splitCount, sizeof(splitCount));
	for(int i = 0; i < mesh->GetMeshSplitCount(); i++)
	{
		int indexSplit = mesh->GetMeshIndexSplitByIndex(i);
		outputFile.write((char*)&indexSplit, sizeof(indexSplit));
	}

	outputFile.close();
}

Mesh *MeshManager::DeserializeMeshFromFile(ID3D12Device* device, char* fileName)
{
	int indexCount = 0;
	std::ifstream inputFile(fileName, std::ios::binary);

	inputFile.read((char *)&indexCount, sizeof(indexCount));

	MeshVertexData *meshData = new MeshVertexData[indexCount];

	inputFile.read((char *)meshData, sizeof(MeshVertexData) * indexCount);

	int indexSplitCount = 0;
	inputFile.read((char *)&indexSplitCount, sizeof(indexSplitCount));

	DynamicArray<int> indexSplits(indexSplitCount);

	for(int i = 0; i < indexSplitCount; i++)
	{
		int indexNum = 0;
		inputFile.read((char *)&indexNum, sizeof(indexNum));
		indexSplits.Add(indexNum);
	}

	inputFile.close();

	Mesh *newMesh = new Mesh();
	newMesh->InitializeWithMeshInfo(device, indexCount, indexCount, meshData, indexSplits);

	return newMesh;
}

Mesh *MeshManager::LoadFromAssimp(ID3D12Device* device, char *fileName, char *serializationFile)
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

	unsigned int vertCount = 0;
	unsigned int indexCount = 0;
	DynamicArray<int> indexSplits;

	for(unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		vertCount += scene->mMeshes[i]->mNumVertices;
		indexCount += scene->mMeshes[i]->mNumFaces * 3;
		indexSplits.Add(indexCount);
	}

	MeshVertexData *meshData = new MeshVertexData[vertCount];
	unsigned long *indices = new unsigned long[indexCount];

	unsigned int meshDataIndex = 0;
	unsigned int indexIndex = 0;

	for(unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		for(unsigned int f = 0; f < scene->mMeshes[i]->mNumFaces; f++)
		{
			indices[indexIndex] = scene->mMeshes[i]->mFaces[f].mIndices[0] + meshDataIndex;
			indexIndex++;
			indices[indexIndex] = scene->mMeshes[i]->mFaces[f].mIndices[1] + meshDataIndex;
			indexIndex++;
			indices[indexIndex] = scene->mMeshes[i]->mFaces[f].mIndices[2] + meshDataIndex;
			indexIndex++;
		}

		for(unsigned int j = 0; j < scene->mMeshes[i]->mNumVertices; j++)
		{
			aiVector3D vert = scene->mMeshes[i]->mVertices[j];
			aiVector3D normal = scene->mMeshes[i]->mNormals[j];
			aiVector3D texcoord = scene->mMeshes[i]->mTextureCoords[0][j];
			aiVector3D tangent = scene->mMeshes[i]->mTangents[j];
			aiVector3D bitangent = scene->mMeshes[i]->mBitangents[j];

			meshData[meshDataIndex].Position = D3DXVECTOR3(vert.x, vert.y, vert.z);
			meshData[meshDataIndex].Normal = D3DXVECTOR3(normal.x, normal.y, normal.z);
			meshData[meshDataIndex].TexCoord = D3DXVECTOR2(texcoord.x, 1.0f - texcoord.y);
			meshData[meshDataIndex].Tangent = D3DXVECTOR3(tangent.x, tangent.y, tangent.z);
			meshData[meshDataIndex].Binormal = D3DXVECTOR3(bitangent.x, bitangent.y, bitangent.z);

			meshDataIndex++;
		}
	}

	Mesh *mesh = new Mesh();
	mesh->InitializeWithMeshInfo(device, vertCount, indexCount, meshData, indexSplits, indices);

	SerializeMeshToFile(mesh, serializationFile);

	delete [] indices;
	return mesh;
}

void MeshManager::ReleaseMeshes()
{
	for(uint32 i = 0; i < mMeshes.CurrentSize(); i++)
	{
		mMeshes[i]->Release();
		delete mMeshes[i];
	}
	mMeshes.Clear();
	mMeshLookup.clear();
}