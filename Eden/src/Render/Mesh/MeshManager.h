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

	void LoadAllMeshes(Direct3DManager *direct3DManager);
	Mesh *GetMesh(const std::string &meshName);

private:
	Mesh *LoadFromAssimp(Direct3DManager *direct3DManager, char *fileName, char *serializationFile);
	Mesh *LoadFBXMesh(Direct3DManager *direct3DManager, char *fileName, char *serializationFile);
	
	void SerializeMeshToFile(Mesh *mesh, char *fileName);
	Mesh *DeserializeMeshFromFile(Direct3DManager *direct3DManager, char *fileName);
    Mesh *GetGridMesh(Direct3DManager *direct3DManager, uint32 xTiles, uint32 yTiles, float texTileX, float texTileZ);

	std::map<std::string, Mesh*> mMeshLookup;
	DynamicArray<Mesh*> mMeshes;

	ManifestLoader mManifestLoader;
	FBXLoader mFBXLoader;
};