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

