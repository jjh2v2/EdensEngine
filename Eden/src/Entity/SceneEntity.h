#pragma once
#include "Render/Mesh/Mesh.h"
#include "Render/Material/Material.h"

class SceneEntity
{
public:
	SceneEntity(Mesh *mesh, Material *material);
	~SceneEntity();

	Material *GetMaterial() { return mMaterial; }
	Vector3 GetPosition() { return mPosition; }
	Vector3 GetRotation() { return mRotation; }
	Vector3 GetScale() { return mScale; }
	void SetPosition(Vector3 position) { mPosition = position; }
	void SetRotation(Vector3 rotation) { mRotation = rotation; }
	void SetScale(Vector3 scale) { mScale = scale; }

	void Update();
	void Render(RenderPassContext *renderPassContext);

private:
    D3DXMATRIX GetWorldMatrix(const Vector3 &position, const Vector3 &rotation, const Vector3 &scale);

	Mesh *mMesh;
	Material *mMaterial;

	Vector3 mPosition;
	Vector3 mRotation;
	Vector3 mScale;
};