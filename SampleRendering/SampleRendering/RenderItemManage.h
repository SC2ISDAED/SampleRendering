#pragma once
#include "Common/d3dUtil.h"
#include "D3d12SDKLayers.h"
#include "Common/GeometryGenerator.h"
#include "RenderItem.h"
/*
	��Ҫ�����������������һЩ��ģ�͵����ݴ������ύ��GPU�ϣ������ܹ������Ͱ󶨲���
	֮�����ʹ�ö��̵߳ķ�ʽ����
*/

class GameTimer;
enum class RenderLayer :int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Sphere,
	GpuWaves,
	Tessellation,
	SkyCube,
	Debug,
	Depth,
	Count
};

class RenderItemManage
{
public:
	RenderItemManage(ID3D12Device* Device,
		ID3D12GraphicsCommandList* cmdList)
	{
		BuildGeomery(Device, cmdList);
		BuildBoxGeometry(Device, cmdList);
		BuildMaterial();
		BuildRenderItem();
	}
	RenderItemManage(const RenderItemManage & rhs) = delete;
	RenderItemManage & operator =(const RenderItemManage & rhs) = delete;
	virtual ~RenderItemManage()
	{

	}
	void Draw(ID3D12GraphicsCommandList *comdList, RenderLayer layer, ID3D12Resource* InsatanceBuffer);

	std::vector<std::unique_ptr<RenderItem>>mAllRitems;
	std::vector<RenderItem *> mRenderItemLayer[(int)RenderLayer::Count];
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
	int getMaxInstanceNum() { return maxInstanceNum; }
private:

	int maxInstanceNum = 0;
	void BuildGeomery(ID3D12Device* Device,ID3D12GraphicsCommandList * cmdList);
	void BuildBoxGeometry(ID3D12Device* Device, ID3D12GraphicsCommandList * cmdList);
	void BuildMaterial();
	void BuildRenderItem();

};