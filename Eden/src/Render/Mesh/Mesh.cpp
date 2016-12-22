#include "Render/Mesh/Mesh.h"
#include "Render/DirectX/Direct3DManager.h"

Mesh::Mesh(Direct3DManager *direct3DManager, uint32 vertexCount, uint32 indexCount, MeshVertexData *meshData, DynamicArray<uint32> &splits, uint64 *indices)
{
	Application::Assert(vertexCount > 0 && indexCount > 0);

	mVertexCount = vertexCount;
	mIndexCount = indexCount;
	mMeshVertices = meshData;
	mMeshIndices = indices;

	if (!mMeshIndices)
	{
		mMeshIndices = new uint64[mIndexCount];
		for (uint32 i = 0; i < mVertexCount; i++)
		{
			mMeshIndices[i] = i;
		}
	}

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

	Vector3 boundMin = mMeshVertices[0].Position;
	Vector3 boundMax = mMeshVertices[0].Position;

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


/*void Mesh::Render(ID3D11DeviceContext* deviceContext, int subMeshIndex)
{
	uint32 stride = sizeof(MeshVertexData);
	uint32 offset = 0;

	int indexOffset = 0;
	int indexCount = mIndexCount;
	
	if(mIndexSplits.CurrentSize() > 1)
	{
		if(subMeshIndex > 0)
		{
			indexOffset = mIndexSplits[subMeshIndex-1];
		}

		indexCount = mIndexSplits[subMeshIndex] - indexOffset;
	}

	deviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(mRenderTopology);
	deviceContext->DrawIndexed(indexCount, indexOffset, 0);
}*/