#include "IBLSpecular.h"
#include "TextureManage.h"
#include "PSOManage.h"
#include "RenderItemManage.h"
#include "FrameResource.h"
using namespace DirectX;
void IBLSpecular::OnResize(UINT newWidth, UINT newHeight)
{
	if ((mWidth != newWidth) || (mHeight != newHeight))
	{
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResource();

		BuildDescriptors();
	}
}

CD3DX12_GPU_DESCRIPTOR_HANDLE IBLSpecular::Srv()
{
	return mhGpuSrv;
}

ID3D12Resource * IBLSpecular::Reousrce()
{
	return mRealResource.Get();
}


void IBLSpecular::BuildDescriptors()
{

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = 36;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(mdevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors =1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(mdevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvHeap)));




	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 6;//每个texturecube需要分别6层
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hSTV(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
	mhCpuSrv = hSTV;
	mdevice->CreateShaderResourceView(mRealResource.Get(), &srvDesc, mhCpuSrv);

	//创建所需要的36个描述符
	CD3DX12_CPU_DESCRIPTOR_HANDLE hRTV(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (int j=0;j<6;j++)
	{
	
		for (int i = 0; i < 6; ++i)
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Format = mFormat;
			//根据官方文档对于这几个Slice 描述，这里应该使用的是 MipSlice
			rtvDesc.Texture2DArray.MipSlice = j;
			rtvDesc.Texture2DArray.PlaneSlice = 0;
			rtvDesc.Texture2DArray.FirstArraySlice = i ;
			
			rtvDesc.Texture2DArray.ArraySize = 1;

			mhCpuRtv[i + j * 6] = hRTV;
			HRESULT Result = mdevice->GetDeviceRemovedReason();;
			mdevice->CreateRenderTargetView(mRealResource.Get(), &rtvDesc, mhCpuRtv[i+j*6]);
			
			hRTV.Offset(1, mRtvSize);
		}
	}

}

void IBLSpecular::BuildDepthStencil()
{
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(mdevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

	mDSV = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mDsvHeap->GetCPUDescriptorHandleForHeapStart(),
		0,
		mDsvSize);

	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mCubeMapSize;
	depthStencilDesc.Height = mCubeMapSize;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 6;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(mdevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mCubeDepthStencilBuffer.GetAddressOf())));

	mdevice->CreateDepthStencilView(mCubeDepthStencilBuffer.Get(), nullptr, mDSV);

	// 初始化资源状态到可写的状态
	mcmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

}

void IBLSpecular::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 36;
	texDesc.MipLevels = 6;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;


	ThrowIfFailed(mdevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mRealResource)));
}

void IBLSpecular::UpdatePassBuffer(int level)
{
	PassConstants mMainPassCB;

	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime =0.0f;
	mMainPassCB.DeltaTime = 0.0f;

	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
	mMainPassCB.Lights[0].Position = { 10.0,15.0,10.0 };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	mMainPassCB.Lights[1].Position = { 10.0,5.0,10.0 };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };
	mMainPassCB.Lights[2].Position = { 5.0,10.0,10.0 };
	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, 1.25f* DirectX::XM_PI, DirectX::XM_PIDIV4);

	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);

	mMainPassCB.FogColor = { 0.5f,0.5f,0.5f,0.3f };
	mMainPassCB.FogStart = { 10.0f };
	mMainPassCB.FogRandge = { 100.0f };
	for (int i = 0; i < 6; ++i)
	{
		PassConstants cubeFacePassCB= mMainPassCB;

		XMMATRIX view = mCamera[i].GetView();
		XMMATRIX proj = mCamera[i].GetProj();

		XMMATRIX viewProj = XMMatrixMultiply(view, proj);
		XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
		XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
		XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
		XMStoreFloat4x4(&cubeFacePassCB.View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&cubeFacePassCB.InvView, XMMatrixTranspose(invView));
		XMStoreFloat4x4(&cubeFacePassCB.Proj, XMMatrixTranspose(proj));
		XMStoreFloat4x4(&cubeFacePassCB.InvProj, XMMatrixTranspose(invProj));
		XMStoreFloat4x4(&cubeFacePassCB.ViewProj, XMMatrixTranspose(viewProj));
	
		cubeFacePassCB.EyePosW = mCamera[i].GetCameraLocation3f();
		cubeFacePassCB.RenderTargetSize = XMFLOAT2((float)mWidth, (float)mHeight);
		cubeFacePassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mWidth, 1.0f / mHeight);
		cubeFacePassCB.roughness = 6-level/6;
		
		auto currPassCB = mFrameResource->PassCB.get();

		// Cube map pass cbuffers are stored in elements 1-6.
		currPassCB->CopyData(1 + i, cubeFacePassCB);
	}
}

//对于Cube 进行处理，生成 Prefilter-map
void IBLSpecular::Process()
{
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	ID3D12DescriptorHeap * descriptorHeaps[] = { mTextureManage->GetCubeMapDescHeap().Get() };

	mcmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mcmdList->SetGraphicsRootSignature(mPSOManage->mRootSignature["Opaque"].Get());
	 

	
	CD3DX12_GPU_DESCRIPTOR_HANDLE skyCube(mTextureManage->GetCubeMapDescHeap()->GetGPUDescriptorHandleForHeapStart());
	mcmdList->SetGraphicsRootDescriptorTable(6, skyCube);

	mcmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRealResource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	for (int i=0;i<6; i++)
	{
		int shrink =  std::pow(2,i);
		mViewport.Height = mCubeMapSize/ shrink;
		mViewport.Width = mCubeMapSize / shrink;
		mScissorRect.right = mCubeMapSize / shrink;
		mScissorRect.bottom= mCubeMapSize / shrink;
		mcmdList->RSSetViewports(1, &mViewport);
		mcmdList->RSSetScissorRects(1, &mScissorRect);
		UpdatePassBuffer(i);
		for (int j=0;j<6;j++)
		{
			mcmdList->ClearRenderTargetView(mhCpuRtv[i*6+j], DirectX::Colors::Black, 0, nullptr);
			mcmdList->ClearDepthStencilView(mDSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);


			mcmdList->OMSetRenderTargets(1, &mhCpuRtv[i*6+j], true, &mDSV);



			auto passCB = mFrameResource->PassCB->Resource();
			auto instanceBuffer = mFrameResource->InstanceBuffer->Resource();

			D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + (j + 1)*passCBByteSize;

			mcmdList->SetGraphicsRootConstantBufferView(0, passCBAddress);

			mcmdList->SetPipelineState(mPSOManage->mPSOs["IBLSpecular"].Get());
			mRenderItemManage->Draw(mcmdList, RenderLayer::SkyCube, instanceBuffer);
		}

	}

	mcmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRealResource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));


}

