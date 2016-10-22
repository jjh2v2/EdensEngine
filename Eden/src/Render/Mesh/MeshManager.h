#pragma once
#include "Render/Mesh/FBX/FBXLoader.h"
#include "Render/Mesh/Mesh.h"
#include "Asset/Manifest/ManifestLoader.h"
#include "Core/Containers/DynamicArray.h"

class MeshManager
{
public:
	MeshManager();
	~MeshManager();

	void LoadAllMeshes(ID3D12Device* device);

	Mesh *GetMesh(std::string meshName);
	void ReleaseMeshes();

private:
	Mesh *LoadFromAssimp(ID3D12Device* device, char *fileName, char *serializationFile);
	Mesh *LoadFBXMesh(ID3D12Device* device, char *fileName, char *serializationFile);
	
	void SerializeMeshToFile(Mesh *mesh, char* fileName);
	Mesh *DeserializeMeshFromFile(ID3D12Device* device, char* fileName);

	std::map<std::string, Mesh*> mMeshLookup;
	DynamicArray<Mesh*> mMeshes;

	ManifestLoader mManifestLoader;
	FBXLoader mFBXLoader;

	static bool mRebuildAllMeshes;
};