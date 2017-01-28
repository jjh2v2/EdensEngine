#pragma once
#include "Camera/Camera.h"
#include "Core/Containers/DynamicArray.h"
#include "Core/Vector/Vector2.h"

class CameraManager
{
public:
	CameraManager(Vector2 screenSize);
	~CameraManager();

	Camera *GetMainCamera() { return mMainCamera; }
	void Update(float delta);

private:
	Camera* mMainCamera;
};