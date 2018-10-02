#pragma once
#include "Camera/CameraManager.h"
#include "Render/Light/LightManager.h"
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
    DirectionalLight *GetSunLight() { return mLightManager->GetSunLight(); }
	
	void ApplyInput(InputManager *inputManager, float delta);
	void Update(float delta);
    void UpdateCamera(float delta);

private:
	GraphicsManager *mGraphicsManager;
	CameraManager *mCameraManager;
    LightManager *mLightManager;

	DynamicArray<SceneEntity*> mSceneEntities;
};