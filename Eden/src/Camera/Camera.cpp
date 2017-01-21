#include "Camera/Camera.h"

Camera::Camera(int32 screenWidth, int32 screenHeight, float screenNear, float screenFar, float fov /* = (float)D3DX_PI / 4.0f */)
{
	mPosition = Vector3(0, 0, 0);
	mRotation = Vector3(0, 0, 0);
	mMoveSpeed = 0.25f;
	mMoveSensitivity = 4.0f;
	mFieldOfView = fov;
	mScreenAspectRatio = (float)screenWidth / (float)screenHeight;
	mScreenWidth = (float)screenWidth;
	mScreenHeight = (float)screenHeight;
	mScreenNear = screenNear;
	mScreenFar = screenFar;

	D3DXMatrixPerspectiveFovLH(&mProjectionMatrix, mFieldOfView, mScreenAspectRatio, screenNear, screenFar);
}

Camera::~Camera()
{

}

void Camera::MoveForward()
{
	mPosition.X += sinf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed;
	mPosition.Y -= sinf(mRotation.X * MathHelper::Radian()) * mMoveSpeed;
	mPosition.Z += cosf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed;
}

void Camera::MoveBackward()
{
	mPosition.X -= sinf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed;
	mPosition.Y += sinf(mRotation.X * MathHelper::Radian()) * mMoveSpeed;
	mPosition.Z -= cosf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed;
}

void Camera::MoveLeft()
{
	mPosition.Z += sinf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed;
	mPosition.X -= cosf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed;
}

void Camera::MoveRight()
{
	mPosition.Z -= sinf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed;
	mPosition.X += cosf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed;
}

void Camera::MouseMove(int32 mouseX, int32 mouseY)
{
	mRotation.Y += mouseX * MathHelper::Radian() * mMoveSensitivity;
	mRotation.X += mouseY * MathHelper::Radian() * mMoveSensitivity;
}

Vector3 Camera::GetDirection()
{
	return Vector3(
		sinf(mRotation.Y * MathHelper::Radian()),
	   -sinf(mRotation.X * MathHelper::Radian()),
		cosf(mRotation.Y * MathHelper::Radian()));
}

void Camera::SetPosition(Vector3 position)
{
	mPosition = position;
}

void Camera::SetRotation(Vector3 rotation)
{
	mRotation = rotation;
}

void Camera::RebuildMatrices()
{
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXVECTOR3 position = mPosition.AsD3DVector3();
	D3DXVECTOR3 lookAt(0.0f, 0.0f, 1.0f);
	float yaw = mRotation.Y * MathHelper::Radian();
	float pitch = mRotation.X * MathHelper::Radian();
	float roll = mRotation.Z * MathHelper::Radian();
	D3DXMATRIX rotationMatrix;

	D3DXMatrixRotationYawPitchRoll(&rotationMatrix, yaw, pitch, roll);
	D3DXVec3TransformCoord(&lookAt, &lookAt, &rotationMatrix);
	D3DXVec3TransformCoord(&up, &up, &rotationMatrix);
	lookAt = position + lookAt;
	D3DXMatrixLookAtLH(&mViewMatrix, &position, &lookAt, &up);
	//D3DXMatrixInverse(&mCameraWorld, NULL, &mViewMatrix);*/
	//BuildFrustum(mScreenFar, mProjectionMatrix);
}