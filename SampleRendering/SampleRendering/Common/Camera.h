#pragma once
#include <direct.h>
#include <DirectXMath.h>
#include "MathHelper.h"
class Camera
{
public:
	Camera();
	~Camera() {}
	//��ȡ���������������������ϵ�µ�λ��
	DirectX::XMFLOAT3 GetCameraLocation3f()const;
	DirectX::XMVECTOR GetCameraLocation()const;
	void SetCameraLocation3f(const DirectX::XMFLOAT3 &newlocation);
	void SetCameraLocation3f(float x, float y, float z);

	//���úͻ�ȡCmaera��base vector
	DirectX::XMFLOAT3 GetCameraUp3f()const;
	DirectX::XMVECTOR GetCameraUp()const;
	DirectX::XMFLOAT3 GetCameraRight3f()const;
	DirectX::XMVECTOR GetCameraRight()const;
	DirectX::XMFLOAT3 GetLookAt3f()const;
	DirectX::XMVECTOR GetLookAt()const;
	//��ȡ��׶�������Ϣ
	float GetNearZ()const;
	float GetFarZ()const;
	float GetFovX()const;
	float GetFovY()const;
	float GetAspect()const;
	//��ȡ�۲�ռ��ʾ�Ľ���Զƽ��
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;
	//��ȡ�۲������ͶӰ����
	DirectX::XMMATRIX GetView()const;
	DirectX::XMMATRIX GetProj()const;

	DirectX::XMFLOAT4X4 GetView4x4f()const;
	DirectX::XMFLOAT4X4 GetProj4x4f()const;
	//������׶��
	void SetLens(float fovY,float aspect,float zn,float zf);
	//ͨ��lookat����View ����
	void SetLookAt(DirectX::FXMVECTOR pos,
		DirectX::FXMVECTOR target,
		DirectX::FXMVECTOR up);
	void SetLookAt(const DirectX::XMFLOAT3 & pos,
		const DirectX::XMFLOAT3 & target,
		const DirectX::XMFLOAT3 & up);

	//����
	void Strafe(float d);
	void Walk(float d);

	//ʵ�����������ת
	void Pitch(float angle);
	void RotateY(float angle);

	//���¹۲����
	void UpdateViewMatrix();

	//���������ϵ���������ռ������
	DirectX::XMFLOAT3 mLocation = { 0.0f,0.0f,0.0f };
private:

	DirectX::XMFLOAT3 mRight = {1.0f,0.0f,0.0f};
	DirectX::XMFLOAT3 mUp = {0.0f,1.0f,0.0f};
	DirectX::XMFLOAT3 mLook = {0.0f,0.0f,1.0f};
	//������׶�������
	float mNearZ = 0.0f;
	float mFarZ = 100.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	//��ʶCamera�Ƿ�õ��޸�
	bool mViewDirty = true;

	DirectX::XMFLOAT4X4 mview = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();
};