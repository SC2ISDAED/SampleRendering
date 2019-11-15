#include "Camera.h"
using namespace DirectX;
Camera::Camera()
{
	SetLens(0.25f*MathHelper::Pi, 1.0f, 1.0f, 1000.0f);
}

DirectX::XMFLOAT3 Camera::GetCameraLocation3f() const
{
	return mLocation;
}

DirectX::XMVECTOR Camera::GetCameraLocation() const
{
	return DirectX::XMLoadFloat3(&mLocation);
}

void Camera::SetCameraLocation3f(const DirectX::XMFLOAT3 &newlocation)
{
	mLocation = newlocation;
}

void Camera::SetCameraLocation3f(float x, float y, float z)
{
	mLocation = DirectX::XMFLOAT3(x, y, z);
}

DirectX::XMFLOAT3 Camera::GetCameraUp3f() const
{
	return mUp;
}

DirectX::XMVECTOR Camera::GetCameraUp() const
{
	return DirectX::XMLoadFloat3(&mUp);
}

DirectX::XMFLOAT3 Camera::GetCameraRight3f() const
{
	return mRight;
}

DirectX::XMVECTOR Camera::GetCameraRight() const
{
	return DirectX::XMLoadFloat3(&mRight);
}

DirectX::XMFLOAT3 Camera::GetLookAt3f() const
{
	return mLook;
}

DirectX::XMVECTOR Camera::GetLookAt() const
{
	return DirectX::XMLoadFloat3(&mLook);
}

float Camera::GetNearZ() const
{
	return mNearZ;
}

float Camera::GetFarZ() const
{
	return mFarZ;
}

float Camera::GetFovX() const
{
	float halfWidth = 0.5f *GetNearWindowWidth();

	return 2.0f*atan(halfWidth/ mNearZ);
}

float Camera::GetFovY() const
{
	return mFovY;
}

float Camera::GetNearWindowWidth() const
{
	return mAspect * mNearWindowHeight;
}

float Camera::GetNearWindowHeight() const
{
	return mNearWindowHeight;
}

float Camera::GetFarWindowWidth() const
{
	return mAspect * mFarWindowHeight;
}

float Camera::GetFarWindowHeight() const
{
	return mFarWindowHeight;
}

DirectX::XMMATRIX Camera::GetView() const
{
	return DirectX::XMLoadFloat4x4(&mview);
}

DirectX::XMMATRIX Camera::GetProj() const
{
	return DirectX::XMLoadFloat4x4(&mProj);
}

DirectX::XMFLOAT4X4 Camera::GetView4x4f() const
{
	return mview;
}

DirectX::XMFLOAT4X4 Camera::GetProj4x4f() const
{
	return mProj;
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	mNearWindowHeight = 2.0f* mNearZ * tanf(0.5*fovY);
	mFarWindowHeight = 2.0f *mFarZ *tanf(0.5f*fovY);
	DirectX::XMMATRIX p = DirectX::XMMatrixPerspectiveFovLH(fovY, mAspect, mNearZ, mFarZ);
	DirectX::XMStoreFloat4x4(&mProj, p);
}

void Camera::SetLookAt(DirectX::FXMVECTOR pos,
	DirectX::FXMVECTOR target,
	DirectX::FXMVECTOR up)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(up, L));
	XMVECTOR U = XMVector3Cross(L, R);
	XMStoreFloat3(&mLook, L);
	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);
	XMStoreFloat3(&mLocation, pos);

	mViewDirty = true;
}

void Camera::SetLookAt(const DirectX::XMFLOAT3 & pos, 
	const DirectX::XMFLOAT3 & target,
	const DirectX::XMFLOAT3 & up)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);
	SetLookAt(P, T, U);

	mViewDirty = true;
}

void Camera::Strafe(float d)
{
	//mPosition +=d*mRight
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR r = XMLoadFloat3(&mRight);
	XMVECTOR p = XMLoadFloat3(&mLocation);
	XMStoreFloat3(&mLocation, XMVectorMultiplyAdd(s, r, p));

	mViewDirty = true;
}

void Camera::Walk(float d)
{
	//mPosition+=d*mlook
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&mLook);
	XMVECTOR p = XMLoadFloat3(&mLocation);
	XMStoreFloat3(&mLocation, XMVectorMultiplyAdd(s, l , p));

	mViewDirty = true;
}

void Camera::Pitch(float angle)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), angle);

	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));

	mViewDirty = true;
}

void Camera::RotateY(float angle)
{
	XMMATRIX R = XMMatrixRotationY(angle);

	XMStoreFloat3(&mRight, XMVector3TransformNormal(XMLoadFloat3(&mRight), R));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));

	mViewDirty = true;
}

void Camera::UpdateViewMatrix()
{
	if (mViewDirty)
	{
		XMVECTOR R = XMLoadFloat3(&mRight);
		XMVECTOR U = XMLoadFloat3(&mUp);
		XMVECTOR L = XMLoadFloat3(&mLook);
		XMVECTOR P = XMLoadFloat3(&mLocation);
		
		//使相机的坐标向量规范化并且相互正交
		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));
	//	R = XMVector3Cross(L, U);
		//填写到view 矩阵中
		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));


		XMStoreFloat3(&mRight, R);
 		XMStoreFloat3(&mUp, U);
 		XMStoreFloat3(&mLook, L);

		mview(0, 0) = mRight.x;
		mview(1, 0) = mRight.y;
		mview(2, 0) = mRight.z;
		mview(3, 0) = x;

		mview(0, 1) = mUp.x;
		mview(1, 1) = mUp.y;
		mview(2, 1) = mUp.z;
		mview(3, 1) = y;

		mview(0, 2) = mLook.x;
		mview(1, 2) = mLook.y;
		mview(2, 2) = mLook.z;
		mview(3, 2) = z;

		mview(0, 3) = 0.0f;
		mview(1, 3) = 0.0f;
		mview(2, 3) = 0.0f;
		mview(3, 3) = 1.0f;

		mViewDirty = false;
	}
}
