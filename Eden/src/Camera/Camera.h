#pragma once
#include "Core/Vector/Vector3.h"

class Camera
{
public:
	Camera(int32 screenWidth, int32 screenHeight, float screenNear, float screenFar, float fov = (float)D3DX_PI / 4.0f);
	~Camera();

	Vector3 GetPosition() { return mPosition; }
	Vector3 GetRotation() { return mRotation; }
	Vector3 GetDirection();
	void SetPosition(Vector3 position);
	void SetRotation(Vector3 rotation);

	void RebuildMatrices();

	void MoveForward();
	void MoveBackward();
	void MoveLeft();
	void MoveRight();
	void MouseMove(int32 mouseX, int32 mouseY);

	const D3DXMATRIX& GetViewMatrix() { return mViewMatrix; }
	const D3DXMATRIX& GetProjectionMatrix() { return mProjectionMatrix; }

private:
	Vector3 mPosition;
	Vector3 mRotation;
	D3DXMATRIX mViewMatrix;
	D3DXMATRIX mProjectionMatrix;

	float mMoveSpeed;
	float mMoveSensitivity;

	float mScreenWidth;
	float mScreenHeight;
	float mScreenNear;
	float mScreenFar;
	float mScreenAspectRatio;
	float mFieldOfView;
};