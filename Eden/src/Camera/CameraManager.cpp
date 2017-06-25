#include "Camera/CameraManager.h"

CameraManager::CameraManager(Vector2 screenSize)
{
	Camera::CameraScreenSettings screenSettings;
	screenSettings.Width = screenSize.X;
	screenSettings.Height = screenSize.Y;
	screenSettings.Near = CAMERA_DEFAULT_NEAR;
	screenSettings.Far = CAMERA_DEFAULT_FAR;
	screenSettings.AspectRatio = screenSize.X / screenSize.Y;
	screenSettings.FieldOfView = MathHelper::Radian() * CAMERA_DEFAULT_FOV;
	mMainCamera = new Camera(screenSettings, CAMERA_DEFAULT_MOVE_SPEED, CAMERA_DEFAULT_MOVE_SENSITIVITY);
}

CameraManager::~CameraManager()
{
	delete mMainCamera;
}

void CameraManager::OnScreenChanged(Vector2 screenSize)
{
    Camera::CameraScreenSettings screenSettings = mMainCamera->GetScreenSettings();
    screenSettings.Width = screenSize.X;
    screenSettings.Height = screenSize.Y;
    screenSettings.AspectRatio = screenSize.X / screenSize.Y;

	mMainCamera->OnScreenChanged(screenSettings);
}

void CameraManager::Update(float delta)
{
	mMainCamera->RebuildViewMatrix();
}