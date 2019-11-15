#include "IBLProcess.h"
#include "TextureManage.h"
#include "PSOManage.h"
#include "RenderItemManage.h"
#include "FrameResource.h"
void IBLPorcess::OnResize(UINT newWidth, UINT newHeight)
{
	if ((mWidth != newWidth) || (mHeight != newHeight))
	{
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResource();

		BuildDescriptors();
	}
}

CD3DX12_GPU_DESCRIPTOR_HANDLE IBLPorcess::Srv()
{
	return mhGpuSrv;
}

ID3D12Resource * IBLPorcess::Reousrce()
{
	return mRealResource.Get();
}


void IBLPorcess::BuildDescriptors()
{

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = 6;
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
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hSTV(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
	mhCpuSrv = hSTV;
	mdevice->CreateShaderResourceView(mRealResource.Get(), &srvDesc, mhCpuSrv);


	CD3DX12_CPU_DESCRIPTOR_HANDLE hRTV(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < 6; ++i)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Format = mFormat;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = i;

		rtvDesc.Texture2DArray.ArraySize = 1;

		mhCpuRtv[i] = hRTV;

		mdevice->CreateRenderTargetView(mRealResource.Get(), &rtvDesc, mhCpuRtv[i]);

		hRTV.Offset(1,mRtvSize);
	}
}

void IBLPorcess::BuildDepthStencil()
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

	// Transition the resource from its initial state to be used as a depth buffer.
	mcmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

}

void IBLPorcess::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 6;
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

void IBLPorcess::Process()
{
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	ID3D12DescriptorHeap * descriptorHeaps[] = { mTextureManage->GetCubeMapDescHeap().Get() };

	mcmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mcmdList->SetGraphicsRootSignature(mPSOManage->mRootSignature["Opaque"].Get());

	mcmdList->RSSetViewports(1, &mViewport);
	mcmdList->RSSetScissorRects(1, &mScissorRect);

	CD3DX12_GPU_DESCRIPTOR_HANDLE skyCube(mTextureManage->GetCubeMapDescHeap()->GetGPUDescriptorHandleForHeapStart());
	mcmdList->SetGraphicsRootDescriptorTable(6, skyCube);

	mcmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRealResource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	for (int i=0;i<6; i++)
	{

		mcmdList->ClearRenderTargetView(mhCpuRtv[i],DirectX::Colors::Black, 0, nullptr);
		mcmdList->ClearDepthStencilView(mDSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);


		mcmdList->OMSetRenderTargets(1, &mhCpuRtv[i], true, &mDSV);

		auto passCB = mFrameResource->PassCB->Resource();
		auto instanceBuffer = mFrameResource->InstanceBuffer->Resource();

		D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + (i+1)*passCBByteSize;
		mcmdList->SetGraphicsRootConstantBufferView(0, passCBAddress);

		mcmdList->SetPipelineState(mPSOManage->mPSOs["Irradiance"].Get());
		mRenderItemManage->Draw(mcmdList, RenderLayer::SkyCube, instanceBuffer);
	}

	mcmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRealResource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));


}

