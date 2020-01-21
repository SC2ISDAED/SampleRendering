#include "ShadowMapManager.h"
#include "BaseStruct.h"
#include "FrameResource.h"
#include "RenderItemManage.h"
#include "PSOManage.h"
#include "Common/Camera.h"
using namespace DirectX;
class Camera;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap > ShadowMapManager::GetDescriptorHeap()
{
	return mSrvHeap;
}

void ShadowMapManager::Draw(ID3D12GraphicsCommandList *cmdList,
	FrameResource *CurrentFrameResource,
	DirectX::BoundingSphere *mSceneBounds,Camera * camera)
{

	mcamera = camera;
	UpdateMainPass(0, CurrentFrameResource, mSceneBounds);
	cmdList->RSSetViewports(1, &mViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);
	cmdList->SetGraphicsRootSignature(mPSOManage->mRootSignature["ShadowMap"].Get());
	for (int i=0;i< ShadowMapNumber;i++)
	{
		// 将深度转换为可写
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap[i].Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

		// 清楚后缓冲和深度缓冲
		cmdList->ClearDepthStencilView(mhCpuDsv[i],
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		// Set null render target because we are only going to draw to
		// depth buffer.  Setting a null render target will disable color writes.
		// Note the active PSO also must specify a render target count of 0.
		//由于我们只需要渲染深度缓冲，所以可以将渲染目标设置为空，这里就是设置为0就行
		cmdList->OMSetRenderTargets(0, nullptr, false, &mhCpuDsv[i]);

		auto passCB = CurrentFrameResource->PassCB->Resource();
		auto instanceBuffer = CurrentFrameResource->InstanceBuffer->Resource();
		auto matBuffer = CurrentFrameResource->MaterialCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 7 * passCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, passCBAddress);
		cmdList->SetGraphicsRootShaderResourceView(1, instanceBuffer->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

		cmdList->SetPipelineState(mPSOManage->mPSOs["ShadowMap"].Get());
		mRenderItemManage->Draw(cmdList, RenderLayer::Opaque, instanceBuffer);

		// Change back to GENERIC_READ so we can read the texture in a shader.
		//将后缓存状态转换到GENERIC_READ ，如此我们就能在着色器中读取
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap[i].Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON));
	}

}

void ShadowMapManager::BuildDescriptors()
{
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(mdevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = ShadowMapNumber;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(mdevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvHeap)));

	mhCpuSrv.resize(ShadowMapNumber);
	mhGpuSrv.resize(ShadowMapNumber);
	mhCpuDsv.resize(ShadowMapNumber);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;

	
	CD3DX12_CPU_DESCRIPTOR_HANDLE hSTV(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDSV(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUhandle(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	for (int i=0;i<ShadowMapNumber;i++)
	{

		mhCpuSrv[i] = hSTV;
		mhCpuDsv[i] = hDSV;
		mhGpuSrv[i] = GPUhandle;
		mdevice->CreateShaderResourceView(mShadowMap[i].Get(), &srvDesc, mhCpuSrv[i]);
		mdevice->CreateDepthStencilView(mShadowMap[i].Get(), &dsvDesc, mhCpuDsv[i]);
		hSTV.Offset(1, mSrvSize);
		hDSV.Offset(1, mDsvSize);
		GPUhandle.Offset(1, mSrvSize);
	}
}

void ShadowMapManager::BuildResource()
{
	mShadowMap.resize(ShadowMapNumber);

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	for (int i = 0; i < ShadowMapNumber; i++)
	{
		ThrowIfFailed(mdevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&optClear,
			IID_PPV_ARGS(&mShadowMap[i])));
	}

}

void ShadowMapManager::UpdateMainPass(int index, FrameResource *CurrentFrameResource, DirectX::BoundingSphere *mSceneBounds)
{

	XMVECTOR lightDir = XMLoadFloat3(&mlightArray[index].Direction);
	XMVECTOR lightPos = -2.0f*mSceneBounds->Radius*lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds->Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMFLOAT3 mLightPosW;
	XMStoreFloat3(&mLightPosW, lightPos);

	// Transform bounding sphere to light space.
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	// Ortho frustum in light space encloses scene.
	//进行正交视锥体的矩阵生成
	float l = sphereCenterLS.x - mSceneBounds->Radius;
	float b = sphereCenterLS.y - mSceneBounds->Radius;
	float n = sphereCenterLS.z - mSceneBounds->Radius;
	float r = sphereCenterLS.x + mSceneBounds->Radius;
	float t = sphereCenterLS.y + mSceneBounds->Radius;
	float f = sphereCenterLS.z + mSceneBounds->Radius;

	float LightNearZ = 0.0f;
	float LightFarZ = 0.0f;
	LightNearZ = n;
	LightFarZ = f;
	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
	
	XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
	XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();

	XMStoreFloat4x4(&mLightView, lightView);
	XMStoreFloat4x4(&mLightProj, lightProj);

	XMMATRIX view = XMLoadFloat4x4(&mLightView);
	XMMATRIX proj = XMLoadFloat4x4(&mLightProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	UINT w = mWidth;
	UINT h = mHeight;
	PassConstants mShadowPassCB;
	XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));


	mShadowPassCB.EyePosW = mLightPosW;
	mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
	mShadowPassCB.NearZ = LightNearZ;
	mShadowPassCB.FarZ = LightFarZ;

	auto currPassCB = CurrentFrameResource->PassCB.get();
	currPassCB->CopyData(7, mShadowPassCB);

}
/*
本是想用来测试多命令队列的渲染方式，但是目前存在一些理解上的问题
*/
void ShadowMapManager::CreateCommand()
{
	//创建命令队列
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(mdevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
	//创建命令分配器
	ThrowIfFailed(mdevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));
	//创建命令列表
	ThrowIfFailed(mdevice->CreateCommandList(
		0,                             //nodeMask对于仅有一个GPU的系统而言，将此值设定为0
		D3D12_COMMAND_LIST_TYPE_DIRECT,//type 设置命令列表的类型
		mDirectCmdListAlloc.Get(),     //pCommandAllocator 与所创建的命令列表关联的命令分配器
		nullptr,					   //pInitialState ，流水线的初始状态
		IID_PPV_ARGS(mCommandList.GetAddressOf())));//riid 带创建的ID3D12CommandList	的COM ID
	//第一次引用命令列表时需要重置，重置之前需要将其关闭
	mCommandList->Close();
}

