#pragma once
#include "Common/d3dUtil.h"
#include "D3d12SDKLayers.h"
#include "Common/GeometryGenerator.h"
#include "RenderItem.h"
/*
	主要是想用这个类来管理一些简单模型的数据创建，提交到GPU上，并且能够创建和绑定材质
	之后可能使用多线程的方式进行
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