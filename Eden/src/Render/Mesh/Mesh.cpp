#include "Render/Mesh/Mesh.h"

Mesh::Mesh()
{
	mMeshVertices = NULL;
}

Mesh::~Mesh()
{

}

void Mesh::InitializeWithMeshInfo(ID3D12Device* device, int vertexCount, int indexCount, MeshVertexData *meshData, DynamicArray<int> &splits)
{
	mVertexCount = vertexCount;
	mIndexCount = indexCount;
	mMeshVertices = meshData;
	Initialize(device);

	for(uint32 i = 0; i < splits.CurrentSize(); i++)
	{
		mIndexSplits.Add(splits[i]);
	}
}

void Mesh::InitializeWithMeshInfo(ID3D12Device* device, int vertexCount, int indexCount, MeshVertexData *meshData, DynamicArray<int> &splits, unsigned long *indices)
{
	mVertexCount = vertexCount;
	mIndexCount = indexCount;
	mMeshVertices = meshData;
	Initialize(device, indices);

	for(uint32 i = 0; i < splits.CurrentSize(); i++)
	{
		mIndexSplits.Add(splits[i]);
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

	for(int i = 1; i < mVertexCount; i++)
	{
		if(mMeshVertices[i].Position.X < boundMin.X)
		{
			boundMin.X = mMeshVertices[i].Position.X;
		}
		if(mMeshVertices[i].Position.Y < boundMin.Y)
		{
			boundMin.Y = mMeshVertices[i].Position.Y;
		}
		if(mMeshVertices[i].Position.Z < boundMin.Z)
		{
			boundMin.Z = mMeshVertices[i].Position.Z;
		}

		if(mMeshVertices[i].Position.X > boundMax.X)
		{
			boundMax.X = mMeshVertices[i].Position.X;
		}
		if(mMeshVertices[i].Position.Y > boundMax.Y)
		{
			boundMax.Y = mMeshVertices[i].Position.Y;
		}
		if(mMeshVertices[i].Position.Z > boundMax.Z)
		{
			boundMax.Z = mMeshVertices[i].Position.Z;
		}
	}

	mMeshBounds.Set(boundMin.X, boundMin.Y, boundMin.Z, boundMax.X - boundMin.X, boundMax.Y - boundMin.Y, boundMax.Z - boundMin.Z);
}

bool Mesh::Initialize(ID3D12Device* device)
{
	if (mVertexCount <= 0 || mIndexCount <= 0)
	{
		return false;
	}

	MeshVertexData* vertices = mMeshVertices;
	
	unsigned long* indices = new unsigned long[mIndexCount];

	RecalculateBounds();

	delete [] indices;
	indices = NULL;
	return true;
}

bool Mesh::Initialize(ID3D12Device* device, unsigned long *indices)
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

void Mesh::Release()
{
	if(mMeshVertices)
	{
		delete [] mMeshVertices;
		mMeshVertices = NULL;
	}
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