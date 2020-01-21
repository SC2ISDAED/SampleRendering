#include "RenderBase.h"
#include <windowsx.h>
#include "Gbuffer.h"
#include "PSOManage.h"
#include "TextureManage.h"
#include "RenderItemManage.h"
#include "IBLProcess.h"
#include "IBLSpecular.h"
#include "FrameResource.h"
#include "Common/Camera.h"
#include "ShadowMapManager.h"
#include "TAAPass.h"
#include "iostream"
#include "thread"
#include "future"

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;
const int gNumFrameResources = 3;

/*
��Ϊ�����߳���Ҫ֪��ִ�к����ĵ�ַ���������Ŀ��ͨ������������Ա�������Ǿ�̬��ַ��������Ҫ����ʱȷ��
����ʱ��������Ԫ�����ķ�ʽ��ִ��ʵ���ĳ�Ա����
*/
UINT  BuildLightThread(void* arg) {
	return static_cast<RenderBase*>(arg)->BuildLight();
}
UINT  BuildPSOThread(void* arg) {
	RenderBase* base = static_cast<RenderBase*>(arg);
	 base->mPSOManage=make_shared<PSOManage>(base->md3dDevice, base->m4xMsaaState);
	 return 0;
}
UINT  BuildRenderItemThread(void* arg) {
	RenderBase* base = static_cast<RenderBase*>(arg);
	base->mRenderItemManage = make_shared<RenderItemManage>(base->md3dDevice.Get(), base->mCommandList.Get());
	return 0;
}

UINT  BuildTextureManagerThread(void* arg) {
	vector<string > textrueName =
	{
		"StreakedAlbedo",
		"StreakedNormal",
		"StreakedMetal",
		"StreakedRoughness",
		"StreakedAO",

		"bambooAlbedo",
		"bambooNormal",
		"bambooMetal",
		"bambooRoughness",
		"bambooAO",

		"greasyAlbedo",
		"greasyNormal",
		"greasyMetal",
		"greasyRoughness",
		"greasyAO",

		"rustAlbedo",
		"rustNormal",
		"rustMetal",
		"rustRoughness",
		"rustAO",

		"mahogfloorAlbedo",
		"mahogfloorNormal",
		"mahogfloorMetal",
		"mahogfloorRoughness",
		"mahogfloorAO",

		"LUT",
		"grassCube",

	};
	std::vector<std::wstring> filePath =
	{
		L"Textures/StreakedAlbedo.dds",
		L"Textures/StreakedNormal.dds",
		L"Textures/StreakedMetalness.dds",
		L"Textures/StreakedRoughess.dds",
		L"Textures/StreakedAO.dds",

		L"Textures/bamboo-wood-semigloss-albedo.dds",
		L"Textures/bamboo-wood-semigloss-normal.dds",
		L"Textures/bamboo-wood-semigloss-metal.dds",
		L"Textures/bamboo-wood-semigloss-roughness.dds",
		L"Textures/bamboo-wood-semigloss-ao.dds",

		L"Textures/greasy-metal-pan1-albedo.dds",
		L"Textures/greasy-metal-pan1-normal.dds",
		L"Textures/greasy-metal-pan1-metal.dds",
		L"Textures/greasy-metal-pan1-roughness.dds",
		L"Textures/greasy-metal-pan1-albedo.dds",

		L"Textures/rust-panel-albedo.dds",
		L"Textures/rust-panel-normal-ue.dds",
		L"Textures/rust-panel-metalness.dds",
		L"Textures/rust-panel-roughness.dds",
		L"Textures/rust-panel-ao.dds",

		L"Textures/mahogfloor_basecolor.dds",
		L"Textures/mahogfloor_normal.dds",
		L"Textures/white1x1.dds",
		L"Textures/mahogfloor_roughness.dds",
		L"Textures/mahogfloor_AO.dds",


		L"Textures/ibl_brdf_lut.dds",
		L"Textures/sunsetcube1024.dds"
	};
	RenderBase* base = static_cast<RenderBase*>(arg);
	base->mTextureManage = make_shared<TextureManage>(base->md3dDevice.Get(), base->mTextureCommandList.Get(),
		textrueName, filePath, 26, 0, 1, base->mCbvSrvUavDescriptorSize);

	return 0;
}

RenderBase::~RenderBase()
{

	if (md3dDevice != nullptr)
	{
		FlushCommandQueue();
	}

}


bool RenderBase::initDirect3D()
{

	CreateDevice();
	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();

	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCamera.mLocation=DirectX::XMFLOAT3(0.0f, 2.0f, -15.0f);

	mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSceneBounds.Radius = sqrtf(30.0f*30.0f + 30.0f*30.0f);
	mPSOManage = make_shared<PSOManage>(md3dDevice, m4xMsaaState);

	//��Ҫ�����б�
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
	ThrowIfFailed(mTextureCommandList->Reset(mTextureCmdListAlloc.Get(), nullptr));
	//�����ĸ��첽ȥ�ֱ�ִ�г�ʼ��ʱ��Ҫ��׼��
	auto buildLightThread = std::async(BuildLightThread, this);
	auto buildPSOThread = std::async(BuildPSOThread, this);
	auto buildRenderThread = std::async(BuildRenderItemThread, this);
	auto buildTextureThread = std::async(BuildTextureManagerThread, this);
	buildLightThread.get();
	buildPSOThread.get();
	buildRenderThread.get();
	buildTextureThread.get();
	
	//��������������Ӧ��������
	/*LoadTextureAndCreateDescriptroHeaps();*/
	//����RenderItem

	mShadowMapManager = make_shared<ShadowMapManager>(md3dDevice.Get(), 2048, 2048,
		mlightArray, mPSOManage, mRenderItemManage, mCbvSrvUavDescriptorSize, mDsvDescriptorSize);

	CreateFrameResource();
	BuildCubeFaceCamera();
	mFrameResources[0]->InlizeInstancedData(mRenderItemManage->mAllRitems, mCamera);
	

	mIBLSpecular = make_shared<IBLSpecular>(md3dDevice.Get(),
		mCommandList.Get(), mTextureManage,
		mPSOManage, mRenderItemManage, mFrameResources[0].get(), mCubeMapCamera,
		CubeMapSize, CubeMapSize,
		mRtvDescriptorSize, mDsvDescriptorSize);

	mIBLProcess = make_shared<IBLPorcess>(md3dDevice.Get(),
		mCommandList.Get(), mTextureManage,
		mPSOManage, mRenderItemManage, mFrameResources[0].get(),
		CubeMapSize, CubeMapSize,
		mRtvDescriptorSize, mDsvDescriptorSize);

	mGbuffer = make_shared<Gbuffer>(md3dDevice.Get(),
		mCommandList.Get(), mTextureManage,
		mPSOManage, mRenderItemManage, mShadowMapManager,
		mIBLSpecular, mIBLProcess,
		mClientWidth, mClientHeight,
		mRtvDescriptorSize, mDsvDescriptorSize);

	mTAAPassDraw = make_shared<TAAPassDraw>(mPSOManage);

	mTextureCommandList->Close();
	mCommandList->Close();
	

	ID3D12CommandList* cmdsLists[] = {mTextureCommandList.Get(), mCommandList.Get() };
	mDirectQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);



	FlushCommandQueue();
	return true;
}

void RenderBase::CreateCommandObjects()
{
	//�����������
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mDirectQueue)));
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mComputeQueue)));
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCopyQueue)));

	//�������������
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mTextureCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
		IID_PPV_ARGS(mComputeCmdListAlloc.GetAddressOf())));
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,
		IID_PPV_ARGS(mCopyCmdListAlloc.GetAddressOf())));
	//���������б�
	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,                             //nodeMask���ڽ���һ��GPU��ϵͳ���ԣ�����ֵ�趨Ϊ0
		D3D12_COMMAND_LIST_TYPE_DIRECT,//type ���������б������
		mDirectCmdListAlloc.Get(),     //pCommandAllocator ���������������б���������������
		nullptr,					   //pInitialState ����ˮ�ߵĳ�ʼ״̬
		IID_PPV_ARGS(mCommandList.GetAddressOf())));//riid ��������ID3D12CommandList	��COM ID

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,                             
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mTextureCmdListAlloc.Get(),
		nullptr,					  
		IID_PPV_ARGS(mTextureCommandList.GetAddressOf())));


	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,                             //nodeMask���ڽ���һ��GPU��ϵͳ���ԣ�����ֵ�趨Ϊ0
		D3D12_COMMAND_LIST_TYPE_COMPUTE,//type ���������б������
		mComputeCmdListAlloc.Get(),     //pCommandAllocator ���������������б���������������
		nullptr,					   //pInitialState ����ˮ�ߵĳ�ʼ״̬
		IID_PPV_ARGS(mComputeList.GetAddressOf())));//riid ��������ID3D12CommandList	��COM ID

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,                             //nodeMask���ڽ���һ��GPU��ϵͳ���ԣ�����ֵ�趨Ϊ0
		D3D12_COMMAND_LIST_TYPE_COPY,//type ���������б������
		mCopyCmdListAlloc.Get(),     //pCommandAllocator ���������������б���������������
		nullptr,					   //pInitialState ����ˮ�ߵĳ�ʼ״̬
		IID_PPV_ARGS(mCopyList.GetAddressOf())));//riid ��������ID3D12CommandList	��COM ID
	//��һ�����������б�ʱ��Ҫ���ã�����֮ǰ��Ҫ����ر�

	mCommandList->Close();
	mTextureCommandList->Close();

	mComputeList->Close();
	mCopyList->Close();
}

void RenderBase::CreateSwapChain()
{
	//�����ͷŵ�֮ǰ�Ľ�����
	mSwapChain.Reset();
	//��һ����дһ��DXGI_SWAP_CHAIN_DESC�Ľṹ��ʵ��
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	//���ö��ز��������������Լ�ÿ�����صĲ�������
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;//���û�������
	sd.OutputWindow = mhMainWnd;//�����������
	sd.Windowed = true;//ָ��ȫ�����Ǵ���ģʽ
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	//ע�⵽��������Ҫͨ��������ж������ˢ��
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mDirectQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

void RenderBase::CreateDepth()
{
	//�������/ģ�� ����
//������дһ�� D3D12_RESOURCE_DESC �Ľṹ���������� ������Դ
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//������Դ��ά��
	depthStencilDesc.Alignment = 0;                                 //У׼
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;							//������Ϊ��λ��ʾ�������
	depthStencilDesc.MipLevels = 1;									//mipmap�㼶��Ŀ
	depthStencilDesc.Format = mDepthStencilFormat;					//
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;		//���ز������������������Լ�ȡ������
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? m4xMsaaQuality - 1 : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;			//ָ��������
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//����Դ��ص�����ȱ�ʶ

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),//��Դ���ύ�Ķ�����
		D3D12_HEAP_FLAG_NONE,//��Դ���ύ�ĶѶ���ı�ʶ��һ����ô�趨
		&depthStencilDesc,//pDesc ָ��һ��D3D12_RESOURCE_DESC��ʵ��ָ��
		D3D12_RESOURCE_STATE_COMMON,//������Դ�ĵ�ǰ״̬
		&optClear,//pOptimizedClearValue ָ��һ��D3D12_CLEAR_VALUE�ṹ�壬������һ�����������Դ���Ż�ֵ
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));
	//���ô���Դ��ʽ��Ϊ������Դ�ĵ�0mip �㴴��������
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());
	//����Դ�ӳ�ʼ״̬ת����Ȼ�����
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void RenderBase::CreateMSAA()
{
	D3D12_RESOURCE_DESC msaaRTDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		mBackBufferFormat,
		mClientWidth,
		mClientHeight,
		1,
		1,
		msaaCount);
	msaaRTDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE msaaOptimizedClearValue = {};
	msaaOptimizedClearValue.Format = mBackBufferFormat;

	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

	auto device = md3dDevice;
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&msaaRTDesc,
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
		&msaaOptimizedClearValue,
		IID_PPV_ARGS(m_msaaRenderTarget.ReleaseAndGetAddressOf())
	));

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = mBackBufferFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtvHeapHandle.Offset(SwapChainBufferCount, mRtvDescriptorSize);
	m_msaaRTVDescriptor = rtvHeapHandle;
	md3dDevice->CreateRenderTargetView(m_msaaRenderTarget.Get(), &rtvDesc, rtvHeapHandle);
}

void RenderBase::CreateFrameResource()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		//0��Cmaera 1~6 ��CubeCmaera ,7:shadowMap
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			8, (UINT)mRenderItemManage->getMaxInstanceNum(), (UINT)mRenderItemManage->mMaterials.size()));
	}
}

void RenderBase::FlushCommandQueue()
{
	//ÿһ��Χ������ά����һ��UINT64���͵�ֵ���������Χ���������
//�������Ϊ0��ÿ����Ҫ���һ���ĵ�Χ����ʱ�ͼ�1

//����Χ��ֵ���������������ǵ���Χ����
	mCurrentFence++;
	//��������������һ�����������µ�Χ���������
	//������������Ҫ����GPU����������GPU��������������д�Signal����������֮ǰ���������������µ�Χ����
	ThrowIfFailed(mDirectQueue->Signal(mFence.Get(), mCurrentFence));
	// Wait until the GPU has completed commands up to this fence point.
	//�ȴ�GPU��ɵ���Χ����֮ǰ������
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence. 
		//��GPU����Χ���㣨��ִ�е�Signal����ָ��޸�Χ��ֵ�����򼤷��¼�
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		//�ȴ�GPU���е�Χ���¼�����
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}


UINT RenderBase::BuildLight()
{
	mlightArray.resize(1);
	mlightArray[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	for (int i = 0; i < mlightArray.size(); i++)
	{
		XMVECTOR lightDir = XMLoadFloat3(&mlightArray[i].Direction);
		XMVECTOR lightPos = -2.0f*mSceneBounds.Radius*lightDir;
		XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
		XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);


		XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

		XMStoreFloat3(&mlightArray[i].Position, lightPos);
		// Transform bounding sphere to light space.
		XMFLOAT3 sphereCenterLS;
		XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

		// Ortho frustum in light space encloses scene.
		float l = sphereCenterLS.x - mSceneBounds.Radius;
		float b = sphereCenterLS.y - mSceneBounds.Radius;
		float n = sphereCenterLS.z - mSceneBounds.Radius;
		float r = sphereCenterLS.x + mSceneBounds.Radius;
		float t = sphereCenterLS.y + mSceneBounds.Radius;
		float f = sphereCenterLS.z + mSceneBounds.Radius;

		float LightNearZ = 0.0f;
		float LightFarZ = 0.0f;
		LightNearZ = n;
		LightFarZ = f;
		XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

		// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
		XMMATRIX T(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);
		XMMATRIX S = lightView * lightProj*T;
		XMStoreFloat4x4(&mlightArray[i].ShadowTransform, S);
	}
	return 0;
	
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderBase::DepthStencilView() const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* RenderBase::CurrentBackBuffer() const
{
	return mSwapChainBuffer[mCurrentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderBase::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrentBackBuffer,
		mRtvDescriptorSize);
}

void RenderBase::BuildCubeFaceCamera()
{
	mCubeMapCamera.resize(6);
	float x =0.0f;
	float y = 0.0f;
	float z = 0.0f;
	XMFLOAT3 center(x, y, z);
	XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);

	XMFLOAT3 targets[6] =
	{
		XMFLOAT3(x + 1.0f, y, z), // +X
		XMFLOAT3(x - 1.0f, y, z), // -X
		XMFLOAT3(x, y + 1.0f, z), // +Y
		XMFLOAT3(x, y - 1.0f, z), // -Y
		XMFLOAT3(x, y, z + 1.0f), // +Z
		XMFLOAT3(x, y, z - 1.0f)  // -Z
	};

	// Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we
	// are looking down +Y or -Y, so we need a different "up" vector.
	XMFLOAT3 ups[6] =
	{
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // +X
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // -X
		XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y
		XMFLOAT3(0.0f, 0.0f, +1.0f), // -Y
		XMFLOAT3(0.0f, 1.0f, 0.0f),	 // +Z
		XMFLOAT3(0.0f, 1.0f, 0.0f)	 // -Z
	};

	for (int i = 0; i < 6; ++i)
	{
		mCubeMapCamera[i].SetLookAt(center, targets[i], ups[i]);
		mCubeMapCamera[i].SetLens(0.5f*XM_PI, 1.0f, 0.1f, 1000.0f);
		mCubeMapCamera[i].UpdateViewMatrix();
	}

	for (int i = 0; i < 6; ++i)
	{
		PassConstants cubeFacePassCB = mMainPassCB;

		XMMATRIX view = mCubeMapCamera[i].GetView();
		XMMATRIX proj = mCubeMapCamera[i].GetProj();

		XMMATRIX viewProj = XMMatrixMultiply(view, proj);
		XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
		XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
		XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
		XMStoreFloat4x4(&cubeFacePassCB.View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&cubeFacePassCB.InvView, XMMatrixTranspose(invView));
		XMStoreFloat4x4(&cubeFacePassCB.Proj, XMMatrixTranspose(proj));
		XMStoreFloat4x4(&cubeFacePassCB.InvProj, XMMatrixTranspose(invProj));
		XMStoreFloat4x4(&cubeFacePassCB.ViewProj, XMMatrixTranspose(viewProj));

		cubeFacePassCB.EyePosW = mCubeMapCamera[i].GetCameraLocation3f();
		cubeFacePassCB.RenderTargetSize = XMFLOAT2((float)CubeMapSize, (float)CubeMapSize);
		cubeFacePassCB.InvRenderTargetSize = XMFLOAT2(1.0f / CubeMapSize, 1.0f / CubeMapSize);

		auto currPassCB = mFrameResources[0]->PassCB.get();

		// Cube map pass cbuffers are stored in elements 1-6.
		currPassCB->CopyData(1 + i, cubeFacePassCB);
	}

}

void RenderBase::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount+1;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

}

void RenderBase::LoadTextureAndCreateDescriptroHeaps()
{

}

void RenderBase::Update(GameTimer& gt)
{
	OnKeyboardInput(gt);
	mCurrentFrameResourceIndex= (mCurrentFrameResourceIndex + 1) % gNumFrameResources;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence)
	{
		int x = mFence->GetCompletedValue();
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
	UpdatePassCB(gt);
	mCurrentFrameResource->UpdateFrameResource(gt, mRenderItemManage->mAllRitems,
		mMainPassCB,
		mRenderItemManage->mMaterials,
		mCamera);
	Result = md3dDevice->GetDeviceRemovedReason();


}

void RenderBase::UpdatePassCB(GameTimer& gt)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

// 	mMainPassCB.ViewProj._21 = (2.0f - 1.0f) / mClientWidth ;
// 	mMainPassCB.ViewProj._22 = (2.0f - 1.0f) / mClientHeight;
	mMainPassCB.LastViewProj = mMainPassCB.ViewProj;
	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	

	mMainPassCB.EyePosW = mCamera.GetCameraLocation3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	for (int i=0;i<mlightArray.size();i++)
	{
		mMainPassCB.Lights[i].Direction = mlightArray[i].Direction;
		mMainPassCB.Lights[i].Strength = mlightArray[i].Strength;
		mMainPassCB.Lights[i].Position = mlightArray[i].Position;
		XMMATRIX shadowTransform = XMLoadFloat4x4(&mlightArray[i].ShadowTransform);
		XMStoreFloat4x4(&mMainPassCB.Lights[i].ShadowTransform, XMMatrixTranspose(shadowTransform));
	}

	mMainPassCB.FogColor = { 1.0f,0.0f,0.0f,0.3f };
	mMainPassCB.FogStart = { 10.0f };
	mMainPassCB.FogRandge = { 100.0f };

	mMainPassCB.FrameCount = (mMainPassCB.FrameCount + 1) % 8;
}

void RenderBase::Draw(GameTimer& gt)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE targetRTV;
	ID3D12Resource * RenderTarget;

	targetRTV = CurrentBackBufferView();
	RenderTarget = CurrentBackBuffer();

	auto cmdListAlloc = mCurrentFrameResource->CmdListAlloc;
	//���������б�������Ҫ�����������
	ThrowIfFailed(cmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOManage->mPSOs["opaque"].Get()));

	mShadowMapManager->Draw(mCommandList.Get(), mCurrentFrameResource, &mSceneBounds,&mCamera);

	mGbuffer->FirstPass(mCurrentFrameResource, mCommandList.Get());
// 
	mGbuffer->SecondPass(mCurrentFrameResource, mCommandList.Get(),
		 mShadowMapManager,DepthStencilView());
	mTAAPassDraw->TAAPass(mCommandList.Get(), mCurrentFrameResource, mGbuffer->mSrvHeap, mGbuffer->mCurrentResoureceGPUSRV,
		mGbuffer->mHistoryResoureceGPUSRV, mGbuffer->mHistoryResourece.Get(), targetRTV,RenderTarget,mGbuffer->mUVVelocityGPUSRV,DepthStencilView());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RenderTarget,
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	if (BDebugRendering)
	{
		mCommandList->RSSetViewports(1, &mMainScreenViewport);
		mCommandList->RSSetScissorRects(1, &mMainScissorRect);

		mCommandList->OMSetRenderTargets(1, &targetRTV, true, &DepthStencilView());

		auto instanceBuffer = mCurrentFrameResource->InstanceBuffer->Resource();

		mCommandList->SetGraphicsRootSignature(mPSOManage->mRootSignature["Debug"].Get());
		mCommandList->SetPipelineState(mPSOManage->mPSOs["Debug"].Get());

		ID3D12DescriptorHeap* srvHeaps[] = { mGbuffer->GetDescHeap().Get()};
		mCommandList->SetDescriptorHeaps(_countof(srvHeaps), srvHeaps);
		mCommandList->SetGraphicsRootDescriptorTable(0, mGbuffer->GetDescHeap()->GetGPUDescriptorHandleForHeapStart());

		mCommandList->SetGraphicsRootShaderResourceView(1, instanceBuffer->GetGPUVirtualAddress());

		mRenderItemManage->Draw(mCommandList.Get(), RenderLayer::Debug, instanceBuffer);

		ID3D12DescriptorHeap* descriptorHeaps[] = { mGbuffer->GetDescHeap().Get() };
		mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		mCommandList->SetGraphicsRootDescriptorTable(0, mGbuffer->mUVVelocityGPUSRV);


// 
//  		ID3D12DescriptorHeap* DepthHeaps[] = { mShadowMapManager->GetDescriptorHeap().Get() };
//   		mCommandList->SetDescriptorHeaps(_countof(DepthHeaps), DepthHeaps);
//  		
//  		mCommandList->SetGraphicsRootDescriptorTable(0, mShadowMapManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
  
		mRenderItemManage->Draw(mCommandList.Get(), RenderLayer::Depth, instanceBuffer);
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RenderTarget,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mDirectQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	Result = md3dDevice->GetDeviceRemovedReason();
	ThrowIfFailed(mSwapChain->Present(0, 0));
	Result = md3dDevice->GetDeviceRemovedReason();
	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SwapChainBufferCount;
	// 
	mCurrentFrameResource->Fence = ++mCurrentFence;

	mDirectQueue->Signal(mFence.Get(), mCurrentFence);

	Result = md3dDevice->GetDeviceRemovedReason();
}

void RenderBase::ForwardRendering()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE targetRTV;
	ID3D12Resource * RenderTarget;

	targetRTV = CurrentBackBufferView();
	RenderTarget = CurrentBackBuffer();
	


	//����viewport �� scissor ����
	mCommandList->RSSetViewports(1, &mMainScreenViewport);
	mCommandList->RSSetScissorRects(1, &mMainScissorRect);

	// ת����Դ״̬����PRESENT �� RENDER_TARGET
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RenderTarget,
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


	// �����Ȼ����ģ�建��
 	mCommandList->ClearRenderTargetView(targetRTV, Colors::AliceBlue, 0, nullptr);
 	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);



	mCommandList->OMSetRenderTargets(1, &targetRTV, true, &DepthStencilView());



	mCommandList->SetGraphicsRootSignature(mPSOManage->mRootSignature["Opaque"].Get());

	auto passCB = mCurrentFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());

	auto instanceBuffer = mCurrentFrameResource->InstanceBuffer->Resource();

	mCommandList->SetGraphicsRootShaderResourceView(1, instanceBuffer->GetGPUVirtualAddress());

	auto matBuffer = mCurrentFrameResource->MaterialCB->Resource();


	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mTextureManage->GetTex2DDescHeap().Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCommandList->SetGraphicsRootDescriptorTable(3, mTextureManage->GetTex2DDescHeap()->GetGPUDescriptorHandleForHeapStart());

	ID3D12DescriptorHeap* descriptorHeapst[] = { mGbuffer->GetDescHeap().Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeapst), descriptorHeapst);
	mCommandList->SetGraphicsRootDescriptorTable(4, mGbuffer->mhNormalResourceGPUSRV);

	ID3D12DescriptorHeap* SkyCubeHeaps[] = { mTextureManage->GetCubeMapDescHeap().Get() };

	mCommandList->SetDescriptorHeaps(_countof(SkyCubeHeaps), SkyCubeHeaps);
	CD3DX12_GPU_DESCRIPTOR_HANDLE skyCube(mTextureManage->GetCubeMapDescHeap()->GetGPUDescriptorHandleForHeapStart());

	mCommandList->SetGraphicsRootDescriptorTable(6, skyCube);

	ID3D12DescriptorHeap* IBLProcessHeaps[] = { mIBLProcess->GetCubeMapDescHeap().Get() };
	mCommandList->SetDescriptorHeaps(_countof(IBLProcessHeaps), IBLProcessHeaps);
	CD3DX12_GPU_DESCRIPTOR_HANDLE IBLDiffuseCube(mIBLProcess->GetCubeMapDescHeap()->GetGPUDescriptorHandleForHeapStart());

	mCommandList->SetGraphicsRootDescriptorTable(7, IBLDiffuseCube);

	ID3D12DescriptorHeap* IBLSpecularHeaps[] = { mIBLSpecular->GetCubeMapDescHeap().Get() };
	mCommandList->SetDescriptorHeaps(_countof(IBLSpecularHeaps), IBLSpecularHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE IBLSpecularCube(mIBLSpecular->GetCubeMapDescHeap()->GetGPUDescriptorHandleForHeapStart());

	mCommandList->SetGraphicsRootDescriptorTable(8, IBLSpecularCube);
	// 
	mCommandList->SetPipelineState(mPSOManage->mPSOs["opaque"].Get());
	mRenderItemManage->Draw(mCommandList.Get(), RenderLayer::Opaque, instanceBuffer);

	mCommandList->SetPipelineState(mPSOManage->mPSOs["skyCube"].Get());
	mRenderItemManage->Draw(mCommandList.Get(), RenderLayer::SkyCube, instanceBuffer);

	//ת������Դ״̬
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RenderTarget,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	if (false)
	{
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RenderTarget,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));

		// Prepare to copy blurred output to the back buffer.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST));
		// 
		mCommandList->ResolveSubresource(CurrentBackBuffer(), 0, RenderTarget, 0, mBackBufferFormat);
		//	mCommandList->CopyResource(CurrentBackBuffer(), RenderTarget);
			// 	// Transition to PRESENT state.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT));

	}
	else
	{
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RenderTarget,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	}

}

void RenderBase::OnMouseUp(int x, int y, POINT mLastMousePox)
{

}

void RenderBase::OnMouseDown(int x, int y, POINT mLastMousePos)
{
	// Make each pixel correspond to a quarter of a degree.
	float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
	float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));
	mCamera.Pitch(dy);
	mCamera.RotateY(dx);
	
}

void RenderBase::OnMouseMove(int x, int y, POINT mLastMousePos)
{
	float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
	float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));
	mCamera.Pitch(dy);
	mCamera.RotateY(dx);
}


void RenderBase::OnKeyboardInput(GameTimer& gt)
{

	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera.Walk(10.0f*dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-10.0f*dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-10.0f*dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(+10.0f*dt);

	mCamera.UpdateViewMatrix();
}

void RenderBase::CreateDevice()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
  	{
  		ComPtr<ID3D12Debug> debugController;
  		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
  		{
  			debugController->EnableDebugLayer();
  		}
  	}
#endif


	//����D3D12���豸
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));
	//���Դ���Ӳ���豸��Ϊ��ѡ������Կ���
	ComPtr<IDXGIAdapter1> pWarpAdapter;
	IDXGIOutput*	pIOutput = nullptr;
	HRESULT			hrEnumOutput = S_OK;
	HRESULT hardwareResult=S_FALSE;
	for (UINT nAdapterIndex=0;DXGI_ERROR_NOT_FOUND!=mdxgiFactory->EnumAdapters1(nAdapterIndex,&pWarpAdapter);nAdapterIndex++)
	{
		DXGI_ADAPTER_DESC1 stAdapterDesc = {};
		pWarpAdapter->GetDesc1(&stAdapterDesc);

		if (stAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{//������������������豸
			continue;
		}

		//��һ�ַ�����ͨ���ж��Ǹ��Կ��������
// 		hrEnumOutput = pWarpAdapter->EnumOutputs(0, &pIOutput);
// 
// 		if (SUCCEEDED(hrEnumOutput) && nullptr != pIOutput)
// 		{//��������������ʾ�����ͨ���Ǽ��ԣ���ԱʼǱ��������
// 			//���ǽ����Գ�ΪMain Device����Ϊ������������������
// 		
// 		}
// 		else
// 		{//������ʾ����ģ�ͨ���Ƕ��ԣ���ԱʼǱ��������
// 			//�����ö����������������Ⱦ����Ȼ��������Ⱦ����������ῴ������ʹ�õ��ǹ����Դ������
// 			hardwareResult = D3D12CreateDevice(
// 				pWarpAdapter.Get(),//nullptr ��ʾʹ��Ĭ��������(default adapter)
// 				D3D_FEATURE_LEVEL_12_0,//����Ӧ�ó�����ҪӲ����֧�ֵ���͹��ܼ���,
// 				IID_PPV_ARGS(&md3dDevice)//����IDED12Device �ӿڵ�COMID
// 			);
// 		}
		//�ڶ��ּ�ⷽʽ
		D3D12_FEATURE_DATA_ARCHITECTURE stArchitecture = {};
		ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&md3dDevice)));
		ThrowIfFailed(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE
			, &stArchitecture, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE)));
		////��������ͨ���ж��Ƿ�UMA�ܹ�������˭��˭��
		if (stArchitecture.UMA)
		{
			hardwareResult = D3D12CreateDevice(
				pWarpAdapter.Get(),//nullptr ��ʾʹ��Ĭ��������(default adapter)
				D3D_FEATURE_LEVEL_12_0,//����Ӧ�ó�����ҪӲ����֧�ֵ���͹��ܼ���,
				IID_PPV_ARGS(&md3dDevice)//����IDED12Device �ӿڵ�COMID
			);
		}
		else
		{
			
		}
	}



	if (FAILED(hardwareResult))//�������ʧ�ܣ�����˵�WARP�豸
	{
		
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	//����Χ�����һ�ȡ��������С
	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	/* ����̨�����ʽ֧�� 4X MSAA
		Direct3d 11 �ܹ�֧�����е���ȾĿ���ʽ��4X MSAA������ֻ��Ҫ���֧�ֵ�����
	*/
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS
		, &msQualityLevels, sizeof(msQualityLevels)));
	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

}

void RenderBase::OnResize(int clientWidth, int clientHeight)
{
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);
	mClientHeight = clientHeight;
	mClientWidth = clientWidth;

	//�ڸ����κ���Դǰ�Ƚ���ˢ�£�CPU��GPU����ͬ��
	FlushCommandQueue();
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
	//�ͷ�֮ǰ����Դ
	for (int i = 0; i < SwapChainBufferCount; i++)
	{
		mSwapChainBuffer[i].Reset();
	}
	m_msaaRenderTarget.Reset();
	mDepthStencilBuffer.Reset();
	//�����趨swap chain �Ĵ�С
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		mClientWidth, mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrentBackBuffer = 0;

	//--------Ϊ��������ÿһ������������һ��RTV
	//��ȡRTV�ѵ�handle
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	int x = 0;
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		//��ý��������ݵĵ�i��������
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		//Ϊ��ǰ����������һ��RTV
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		//ƫ�Ƶ��������ѵ���һ��������
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
		x++;
	}
	CreateMSAA();
	CreateDepth();
	
	// ִ�� resize ����
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mDirectQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	//�ȴ�GPU���
	FlushCommandQueue();
	//���� viewport transform �Ը��� client area
	mMainScreenViewport.TopLeftX = 0;
	mMainScreenViewport.TopLeftY = 0;
	mMainScreenViewport.Width = clientWidth;
	mMainScreenViewport.Height =clientHeight;
	mMainScreenViewport.MinDepth = 0.0f;
	mMainScreenViewport.MaxDepth = 1.0f;

	mMainScissorRect = { 0,0, clientWidth, clientHeight };

	Result = md3dDevice->GetDeviceRemovedReason();
	for (int i=0;i<4;i++)
	{
		mDebugScreenViewport[i].TopLeftX = i*DebugWidth;
		mDebugScreenViewport[i].TopLeftY = 600;
		mDebugScreenViewport[i].Height = 200;
		mDebugScreenViewport[i].Width = 150;
		mDebugScreenViewport[i].MinDepth = 0.0f;
		mDebugScreenViewport[i].MaxDepth = 1.0f;

		mDebugScissorRect[i] = { i*DebugWidth ,600 ,(i + 1)*DebugWidth,750 };
	}
}

