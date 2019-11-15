#pragma once
#include "Common/MathHelper.h"
#include "Common/d3dUtil.h"
#include "BaseStruct.h"
struct RenderItem
{
	RenderItem() = default;
	//shape�������������Ŀ��ľֲ�λ��������ռ�Ĺ�ϵ����������λ�ã�����
	//�Լ������������е�����
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	//GPU waves render items
	DirectX::XMFLOAT2 DisplacementMapTexelSize = { 1.0f, 1.0f };
	float GridSpatialStep = 1.0f;
	//Dirty ��ʶֻ����Ŀ�������Ѿ����ģ�������Ҫ���³�������
	//��Ϊÿ֡����һ�� object cbuff��������ҪΪÿһ֡���¡���ˣ�������Ҫ���� object data
	//���ҵ�ÿһ֡���º�����NumFramesDirty = gNumFrameResources
	int NumFramesDirty = 3;

	//��������Ӧ�ڴ���Ⱦ���ObjectCB��GPU������������
	UINT ObjCBIndex = -1;

	Material *Mat = nullptr;
	MeshGeometry *Geo = nullptr;

	//Primitive topology   ͼԪ��������
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	std::vector<InstanceData> Instances;


	//DrawIndexedInstanced �Ĳ���
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};