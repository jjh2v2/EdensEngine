#pragma once
#include "Camera/CameraManager.h"

class InputManager;
class GraphicsManager;

class Scene
{
public:
	Scene(GraphicsManager *graphicsManager);
	~Scene();

	CameraManager *GetCameraManager() { return mCameraManager; }
	Camera *GetMainCamera() { return mCameraManager->GetMainCamera(); }
	
	void ApplyInput(InputManager *inputManager, float delta);
	void Update(float delta);
	
private:
	CameraManager *mCameraManager;

};