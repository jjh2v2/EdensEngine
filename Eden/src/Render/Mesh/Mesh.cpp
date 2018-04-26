#include "Render/Mesh/Mesh.h"
#include "Render/DirectX/Direct3DManager.h"

Mesh::Mesh(Direct3DManager *direct3DManager, uint32 vertexCount, uint32 indexCount, MeshVertexData *meshData, DynamicArray<uint32> &splits, uint32 *indices)
{
	Application::Assert(vertexCount > 0 && indexCount > 0);

	mVertexCount = vertexCount;
	mIndexCount = indexCount;
	mMeshVertices = meshData;
	mMeshIndices = indices;

	for (uint32 i = 0; i < splits.CurrentSize(); i++)
	{
		mIndexSplits.Add(splits[i]);
	}

	mVertexBuffer = direct3DManager->GetContextManager()->CreateVertexBuffer(meshData, sizeof(MeshVertexData), vertexCount * sizeof(MeshVertexData));
	mIndexBuffer = direct3DManager->GetContextManager()->CreateIndexBuffer(meshData, indexCount * sizeof(uint32));

	RecalculateBounds();
}

Mesh::~Mesh()
{
	if (mVertexBuffer)
	{
		delete mVertexBuffer;
		mVertexBuffer = NULL;
	}

	if (mIndexBuffer)
	{
		delete mIndexBuffer;
		mIndexBuffer = NULL;
	}

	if (mMeshVertices)
	{
		delete[] mMeshVertices;
		mMeshVertices = NULL;
	}

	if (mMeshIndices)
	{
		delete[] mMeshIndices;
		mMeshIndices = NULL;
	}
}

void Mesh::RecalculateBounds()
{
	if(mVertexCount <= 0)
	{
		return;
	}

	Vector3 boundMin = Vector3(mMeshVertices[0].Position.X, mMeshVertices[0].Position.Y, mMeshVertices[0].Position.Z);
	Vector3 boundMax = Vector3(mMeshVertices[0].Position.X, mMeshVertices[0].Position.Y, mMeshVertices[0].Position.Z);

	for(uint32 i = 1; i < mVertexCount; i++)
	{
		boundMin.X = MathHelper::Min(mMeshVertices[i].Position.X, boundMin.X);
		boundMin.Y = MathHelper::Min(mMeshVertices[i].Position.Y, boundMin.Y);
		boundMin.Z = MathHelper::Min(mMeshVertices[i].Position.Z, boundMin.Z);

		boundMax.X = MathHelper::Max(mMeshVertices[i].Position.X, boundMax.X);
		boundMax.Y = MathHelper::Max(mMeshVertices[i].Position.Y, boundMax.Y);
		boundMax.Z = MathHelper::Max(mMeshVertices[i].Position.Z, boundMax.Z);
	}

	mMeshBounds.Set(boundMin.X, boundMin.Y, boundMin.Z, boundMax.X - boundMin.X, boundMax.Y - boundMin.Y, boundMax.Z - boundMin.Z);
}