#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Vector/Vector4.h"
#include "Core/Vector/Vector3.h"
#include "Core/Vector/Vector2.h"
#include "Core/Containers/DynamicArray.h"
#include "Core/Misc/Box.h"
#include "Render/Buffer/GPUResource.h"

class Direct3DManager;

struct MeshVertexData
{
	Vector4 Position;
	Vector4 TexCoord;
	Vector3 Normal;
	Vector3 Tangent;
	Vector3 Binormal;
	Vector4 Color;

	static void GetTangentAndBinormal(MeshVertexData data1, MeshVertexData data2, MeshVertexData data3, Vector3 &tangent, Vector3 &binormal)
	{
		Vector3 v1, v2;
		Vector2 t1, t2;

		v1 = Vector3(data2.Position.X - data1.Position.X, data2.Position.Y - data1.Position.Y, data2.Position.Z - data1.Position.Z);
		v2 = Vector3(data3.Position.X - data1.Position.X, data3.Position.Y - data1.Position.Y, data3.Position.Z - data1.Position.Z);

		t1 = Vector2(data2.TexCoord.X - data1.TexCoord.X, data2.TexCoord.Y - data1.TexCoord.Y);
		t2 = Vector2(data3.TexCoord.X - data1.TexCoord.X, data3.TexCoord.Y - data1.TexCoord.Y);

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
	Mesh(Direct3DManager *direct3DManager, uint32 vertexCount, uint32 indexCount, MeshVertexData *meshData, DynamicArray<uint32> &splits, uint64 *indices);
	virtual ~Mesh();

	uint32 GetIndexCount() { return mIndexCount; }
	uint32 GetVertexCount() { return mVertexCount; }
	uint32 GetMeshSplitCount() { return mIndexSplits.CurrentSize(); }
	uint32 GetMeshIndexSplitByIndex(uint32 index) { return mIndexSplits[index]; }
	MeshVertexData *GetMeshData() { return mMeshVertices; }
	uint64 *GetIndices() { return mMeshIndices; }

	void RecalculateBounds();
	Box GetBounds(){return mMeshBounds;}

	VertexBuffer *GetVertexBuffer() { return mVertexBuffer; }
	IndexBuffer *GetIndexBuffer() { return mIndexBuffer; }

    bool IsReady() { return mVertexBuffer->GetIsReady() && mIndexBuffer->GetIsReady(); }

protected:

	VertexBuffer *mVertexBuffer;
	IndexBuffer *mIndexBuffer;
	MeshVertexData *mMeshVertices;
	uint64 *mMeshIndices;
	DynamicArray<uint32> mIndexSplits;
	uint32 mVertexCount;
	uint32 mIndexCount;
	Box mMeshBounds;
};