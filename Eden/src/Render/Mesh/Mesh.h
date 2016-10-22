#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Vector/Vector3.h"
#include "Core/Vector/Vector2.h"
#include "Core/Containers/DynamicArray.h"
#include "Core/Misc/Box.h"

struct MeshVertexData
{
	D3DXVECTOR3 Position;
	D3DXVECTOR2 TexCoord;
	D3DXVECTOR3 Normal;
	D3DXVECTOR3 Tangent;
	D3DXVECTOR3 Binormal;
	D3DXVECTOR4 Color;

	static void GetTangentAndBinormal(MeshVertexData data1, MeshVertexData data2, MeshVertexData data3, Vector3 &tangent, Vector3 &binormal)
	{
		Vector3 v1, v2;
		Vector2 t1, t2;

		v1 = Vector3(data2.Position.x - data1.Position.x, data2.Position.y - data1.Position.y, data2.Position.z - data1.Position.z);
		v2 = Vector3(data3.Position.x - data1.Position.x, data3.Position.y - data1.Position.y, data3.Position.z - data1.Position.z);

		t1 = Vector2(data2.TexCoord.x - data1.TexCoord.x, data2.TexCoord.y - data1.TexCoord.y);
		t2 = Vector2(data3.TexCoord.x - data1.TexCoord.x, data3.TexCoord.y - data1.TexCoord.y);

		float div = 1.0f / (t1.X * t2.Y - t2.X * t1.Y);

		tangent.X = (t2.Y * v1.X - t1.Y * v2.X) * div;
		tangent.Y = (t2.Y * v1.Y - t1.Y * v2.Y) * div;
		tangent.Z = (t2.Y * v1.Z - t1.Y * v2.Z) * div;

		binormal.X = (t1.X * v2.X - t2.X * v1.X) * div;
		binormal.Y = (t1.X * v2.Y - t2.X * v1.Y) * div;
		binormal.Z = (t1.X * v2.Z - t2.X * v1.Z) * div;

		tangent = tangent.Normalized();
		binormal = binormal.Normalized();
	}
};

class Mesh
{
public:
	Mesh();
	virtual ~Mesh();
	virtual bool Initialize(ID3D12Device* device);
	virtual bool Initialize(ID3D12Device* device, unsigned long *indices);
	virtual void InitializeWithMeshInfo(ID3D12Device* device, int vertexCount, int indexCount, MeshVertexData *meshData, DynamicArray<int> &splits);
	virtual void InitializeWithMeshInfo(ID3D12Device* device, int vertexCount, int indexCount, MeshVertexData *meshData, DynamicArray<int> &splits, unsigned long *indices);
	virtual void Release();
	//virtual void Render(ID3D12DeviceContext* deviceContext, int subMeshIndex = 0);
	//void SetRenderTopology(D3D11_PRIMITIVE_TOPOLOGY topology){mRenderTopology = topology;}
	//D3D11_PRIMITIVE_TOPOLOGY GetRenderTopology(){return mRenderTopology;}

	int GetIndexCount(){return mIndexCount;}
	int GetMeshSplitCount(){return mIndexSplits.CurrentSize();}
	int GetMeshIndexSplitByIndex(uint32 index){return mIndexSplits[index];}
	MeshVertexData *GetMeshData(){return mMeshVertices;}

	void RecalculateBounds();
	Box GetBounds(){return mMeshBounds;}

protected:
	int mVertexCount;
	int mIndexCount;

	DynamicArray<int> mIndexSplits;

	MeshVertexData *mMeshVertices;
	Box mMeshBounds;
};