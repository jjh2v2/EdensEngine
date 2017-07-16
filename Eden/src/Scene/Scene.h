#pragma once
#include "Camera/CameraManager.h"
#include "Entity/SceneEntity.h"

class InputManager;
class GraphicsManager;

class Scene
{
public:
	Scene(GraphicsManager *graphicsManager);
	~Scene();

	void OnScreenChanged(Vector2 screenSize);

	CameraManager *GetCameraManager() { return mCameraManager; }
	Camera *GetMainCamera() { return mCameraManager->GetMainCamera(); }
	
	void ApplyInput(InputManager *inputManager, float delta);
	void Update(float delta);
	
private:
	GraphicsManager *mGraphicsManager;
	CameraManager *mCameraManager;

	DynamicArray<SceneEntity*> mSceneEntities;
};