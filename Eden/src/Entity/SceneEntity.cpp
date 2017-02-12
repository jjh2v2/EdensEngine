#include "Entity/SceneEntity.h"

SceneEntity::SceneEntity(Mesh *mesh, Material *material)
{
	mMesh = mesh;
	mMaterial = material;
	mPosition = Vector3(0, 0, 0);
	mRotation = Vector3(0, 0, 0);
	mScale = Vector3(1, 1, 1);
}

SceneEntity::~SceneEntity()
{

}

void SceneEntity::Update()
{
	
}

void SceneEntity::Render()
{
	D3DXMATRIX modelMatrix, positionMatrix, rotationMatrix, scalarMatrix;
	D3DXMatrixIdentity(&positionMatrix);
	D3DXMatrixIdentity(&rotationMatrix);
	D3DXMatrixIdentity(&scalarMatrix);
	D3DXMatrixIdentity(&modelMatrix);
	D3DXMatrixTranslation(&positionMatrix, mPosition.X, mPosition.Y, mPosition.Z);
	D3DXMatrixRotationYawPitchRoll(&rotationMatrix, mRotation.Y, mRotation.X, mRotation.Z);
	D3DXMatrixScaling(&scalarMatrix, mScale.X, mScale.Y, mScale.Z);
	D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &scalarMatrix);
	D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &rotationMatrix);
	D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &positionMatrix);

	mMaterial->GetMaterialBuffer()->SetWorldMatrix(modelMatrix);
	mMaterial->CommitConstantBufferChanges();
}