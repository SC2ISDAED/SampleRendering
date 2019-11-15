#pragma once
#include "Common/MathHelper.h"
#include "Common/d3dUtil.h"
#include "BaseStruct.h"
struct RenderItem
{
	RenderItem() = default;
	//shape世界矩阵描述了目标的局部位置与世界空间的关系，它定义了位置，朝向
	//以及在世界坐标中的缩放
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	//GPU waves render items
	DirectX::XMFLOAT2 DisplacementMapTexelSize = { 1.0f, 1.0f };
	float GridSpatialStep = 1.0f;
	//Dirty 标识只是了目标数据已经更改，我们需要更新常量缓冲
	//因为每帧都有一个 object cbuff，我们需要为每一帧更新。因此，我们需要修正 object data
	//并且当每一帧更新后，设置NumFramesDirty = gNumFrameResources
	int NumFramesDirty = 3;

	//索引到对应于此渲染项的ObjectCB的GPU常量缓冲区。
	UINT ObjCBIndex = -1;

	Material *Mat = nullptr;
	MeshGeometry *Geo = nullptr;

	//Primitive topology   图元拓扑类型
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	std::vector<InstanceData> Instances;


	//DrawIndexedInstanced 的参数
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};