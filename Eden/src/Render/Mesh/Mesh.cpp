#include "Render/Mesh/Mesh.h"

Mesh::Mesh(ID3D12Device* device, uint32 vertexCount, uint32 indexCount, MeshVertexData *meshData, DynamicArray<uint32> &splits, uint64 *indices)
{
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

	Initialize(device, indices);

	for (uint32 i = 0; i < splits.CurrentSize(); i++)
	{
		mIndexSplits.Add(splits[i]);
	}
}

Mesh::~Mesh()
{
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

bool Mesh::Initialize(ID3D12Device* device, uint64 *indices)
{
	/*if (mVertexCount <= 0 || mIndexCount <= 0)
	{
		return false;
	}

	MeshVertexData* vertices = mMeshVertices;

	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_BUFFER_DESC indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;
	D3D11_SUBRESOURCE_DATA indexData;
	HRESULT result;

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(MeshVertexData) * mVertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &mVertexBuffer);
	if(FAILED(result))
	{
		return false;
	}

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * mIndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &mIndexBuffer);
	if(FAILED(result))
	{
		return false;
	}

	RecalculateBounds();
	*/
	return true;
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