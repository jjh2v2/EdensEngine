#include "Camera/CameraManager.h"

//TDA: Need to be able to reconstruct this when screen size changes
CameraManager::CameraManager(Vector2 screenSize)
{
	Camera::CameraScreenSettings screenSettings;
	screenSettings.Width = screenSize.X;
	screenSettings.Height = screenSize.Y;
	screenSettings.Near = 0.1f;
	screenSettings.Far = 1000.0f;
	screenSettings.AspectRatio = screenSize.X / screenSize.Y;
	screenSettings.FieldOfView = MathHelper::Radian() * 60.0f;
	mMainCamera = new Camera(screenSettings, 0.5f, 200.0f);
}

CameraManager::~CameraManager()
{
	delete mMainCamera;
}

void CameraManager::Update(float delta)
{
	mMainCamera->RebuildViewMatrix();
}