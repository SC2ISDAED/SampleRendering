#pragma once
#include <direct.h>
#include <DirectXMath.h>
#include "MathHelper.h"
class Camera
{
public:
	Camera();
	~Camera() {}
	//获取和设置摄像机在世界坐标系下的位置
	DirectX::XMFLOAT3 GetCameraLocation3f()const;
	DirectX::XMVECTOR GetCameraLocation()const;
	void SetCameraLocation3f(const DirectX::XMFLOAT3 &newlocation);
	void SetCameraLocation3f(float x, float y, float z);

	//设置和获取Cmaera的base vector
	DirectX::XMFLOAT3 GetCameraUp3f()const;
	DirectX::XMVECTOR GetCameraUp()const;
	DirectX::XMFLOAT3 GetCameraRight3f()const;
	DirectX::XMVECTOR GetCameraRight()const;
	DirectX::XMFLOAT3 GetLookAt3f()const;
	DirectX::XMVECTOR GetLookAt()const;
	//获取视锥体相关信息
	float GetNearZ()const;
	float GetFarZ()const;
	float GetFovX()const;
	float GetFovY()const;
	float GetAspect()const;
	//获取观察空间表示的近，远平面
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;
	//获取观察矩阵与投影矩阵
	DirectX::XMMATRIX GetView()const;
	DirectX::XMMATRIX GetProj()const;

	DirectX::XMFLOAT4X4 GetView4x4f()const;
	DirectX::XMFLOAT4X4 GetProj4x4f()const;
	//设置视锥体
	void SetLens(float fovY,float aspect,float zn,float zf);
	//通过lookat设置View 矩阵
	void SetLookAt(DirectX::FXMVECTOR pos,
		DirectX::FXMVECTOR target,
		DirectX::FXMVECTOR up);
	void SetLookAt(const DirectX::XMFLOAT3 & pos,
		const DirectX::XMFLOAT3 & target,
		const DirectX::XMFLOAT3 & up);

	//设置
	void Strafe(float d);
	void Walk(float d);

	//实质摄像机的旋转
	void Pitch(float angle);
	void RotateY(float angle);

	//更新观察矩阵
	void UpdateViewMatrix();

	//摄像机坐标系相对于世界空间的坐标
	DirectX::XMFLOAT3 mLocation = { 0.0f,0.0f,0.0f };
private:

	DirectX::XMFLOAT3 mRight = {1.0f,0.0f,0.0f};
	DirectX::XMFLOAT3 mUp = {0.0f,1.0f,0.0f};
	DirectX::XMFLOAT3 mLook = {0.0f,0.0f,1.0f};
	//设置视锥体的属性
	float mNearZ = 0.0f;
	float mFarZ = 100.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	//标识Camera是否得到修改
	bool mViewDirty = true;

	DirectX::XMFLOAT4X4 mview = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();
};