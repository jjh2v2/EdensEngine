#pragma once
#include "Camera/Camera.h"
#include "Core/Containers/DynamicArray.h"
#include "Core/Vector/Vector2.h"

#define CAMERA_DEFAULT_NEAR			        0.1f
#define CAMERA_DEFAULT_FAR		         1000.0f
#define CAMERA_DEFAULT_FOV				   60.0f
#define CAMERA_DEFAULT_MOVE_SPEED			0.5f
#define CAMERA_DEFAULT_MOVE_SENSITIVITY   200.0f

class CameraManager
{
public:
	CameraManager(Vector2 screenSize);
	~CameraManager();

	void OnScreenChanged(Vector2 screenSize);

	Camera *GetMainCamera() { return mMainCamera; }
	void Update(float delta);

private:
	Camera* mMainCamera;
};