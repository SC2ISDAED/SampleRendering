#include "FrameResource.h"
#include "Common/Camera.h"
#include"RenderItem.h"
#include "GameTimer.h"
using namespace  DirectX;
FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount,
	UINT MaterialCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);

	MaterialCB = std::make_unique<UploadBuffer<MaterialData>>(device, MaterialCount, false);
	InstanceBuffer = std::make_unique<UploadBuffer<InstanceData>>(device, objectCount, false);
	
}

void FrameResource::UpdateFrameResource(const GameTimer &gt,
	const std::vector<std::unique_ptr<RenderItem>> &mAllRitems,
	PassConstants &mMainPassCB,
	const std::unordered_map<std::string,
	std::unique_ptr<Material>> & mMaterials,
	const Camera &mCamera)
{
	UpdateInstancedData(gt,mAllRitems, mCamera);
//	InlizeInstancedData(mAllRitems, mCamera);
	UpdateMainPassCBS(mMainPassCB);
	UpdateMaterialCBS(gt, mMaterials);
}

void FrameResource::UpdateInstancedData(const GameTimer &gt, const std::vector<std::unique_ptr<RenderItem>> &mAllRitems,const Camera& mCamera)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

	auto currObjectCB = InstanceBuffer.get();

	XMMATRIX world = XMLoadFloat4x4(&mAllRitems[0]->Instances[0].World);
	XMStoreFloat4x4(&mAllRitems[0]->Instances[0].LastWorld, world);
	XMStoreFloat4x4(&mAllRitems[0]->Instances[0].World, XMMatrixTranslation(0.0f, 0.0f, 1.0f*gt.TotalTime())*XMMatrixScaling(10.0f, 10.0f, 10.0f));
		
	int visibleInstanceCount = 0;
	for (auto& e : mAllRitems)
	{
		const auto& instanceData = e->Instances;


		for (UINT i = 0; i < (UINT)instanceData.size(); ++i)
		{
			XMMATRIX world = XMLoadFloat4x4(&instanceData[i].World);
			XMMATRIX Lastworld = XMLoadFloat4x4(&instanceData[i].LastWorld);
			XMMATRIX texTransform = XMLoadFloat4x4(&instanceData[i].TexTransform);

			XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);


			XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);


			InstanceData data;

			XMStoreFloat4x4(&data.LastWorld, XMMatrixTranspose(Lastworld));//暂时未完成场景中移动物体的TAA

			XMStoreFloat4x4(&data.World, XMMatrixTranspose(world));

			

			XMStoreFloat4x4(&data.TexTransform, XMMatrixTranspose(texTransform));
			data.MaterialIndex = instanceData[i].MaterialIndex;

			currObjectCB->CopyData(visibleInstanceCount++, data);
		}

	}
}

void FrameResource::InlizeInstancedData(const std::vector<std::unique_ptr<RenderItem>> &mAllRitems, const Camera& mCamera)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

	auto currObjectCB = InstanceBuffer.get();

	int visibleInstanceCount = 0;
	for (auto& e : mAllRitems)
	{
		const auto& instanceData = e->Instances;


		for (UINT i = 0; i < (UINT)instanceData.size(); ++i)
		{
			XMMATRIX world = XMLoadFloat4x4(&instanceData[i].World);
			XMMATRIX texTransform = XMLoadFloat4x4(&instanceData[i].TexTransform);

			XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);


			XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);


			InstanceData data;

			

			XMStoreFloat4x4(&data.World, XMMatrixTranspose(world));

			XMStoreFloat4x4(&data.LastWorld, XMMatrixTranspose(world));//暂时未完成场景中移动物体的TAA

			XMStoreFloat4x4(&data.TexTransform, XMMatrixTranspose(texTransform));
			data.MaterialIndex = instanceData[i].MaterialIndex;

			currObjectCB->CopyData(visibleInstanceCount++, data);
		}

	}
}

void FrameResource::UpdateMainPassCBS(PassConstants &mMainPassCB )
{
	auto currPassCB = PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);

}

void FrameResource::UpdateMaterialCBS(const GameTimer &gt,
	const std::unordered_map<std::string, std::unique_ptr<Material>> & mMaterials)
{
	auto currMaterialCB = MaterialCB.get();

	for (auto&e : mMaterials)
	{
		Material * mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialData matData;
			matData.AlbeoMapIndex = mat->AlbeoSrvHeapIndex;
			matData.FresnelR0 = mat->FresnelR0;
			matData.RoughnessMapIndex = mat->RoughnessSrvHeapIndex;
			matData.AOMapIndex = mat->AOMapSrvHeapIndex;
			matData.NormlMapIndex = mat->NormalSrvHeapIndex;
			matData.MetalMapIndex = mat->MetalSrvHeapIndex;
			
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			currMaterialCB->CopyData(mat->MatCBIndex, matData);

			mat->NumFramesDirty--;
		}
	}
}

void FrameResource::BuildCubeMapCamera(PassConstants * cubeFacePassCB)
{
	auto currPassCB = PassCB.get();
	for (int i = 0; i < 6; ++i)
	{
		currPassCB->CopyData(1 + i, cubeFacePassCB[i]);
	}
}
