#include "Gbuffer.h"
#include "PSOManage.h"
#include "FrameResource.h"
#include "TextureManage.h"
#include "RenderItemManage.h"
#include "ShadowMapManager.h"
#include "IBLProcess.h"
#include "IBLSpecular.h"
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Gbuffer::GetDescHeap()
{
	return mSrvHeap;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Gbuffer::NormalSrv()
{
	return mhNormalResourceGPUSRV;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Gbuffer::PositionSrv()
{
	return mhPositionResourceGPUSRV;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Gbuffer::baseColorAlbedoSrv()
{
	return mhbaseColorAlbedoResourceGPUSRV;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Gbuffer::RoughnessMetallicAOF0Srv()
{
	return mhRoughnessMetallicAOF0ResourceGPUSRV;
}

void Gbuffer::FirstPass(FrameResource *FrameResources, ID3D12GraphicsCommandList *cmdList)
{
	//重设裁剪窗口 和 viewport
	cmdList->RSSetViewports(1, &mViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);

	cmdList->ClearDepthStencilView(mDSV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	cmdList->ClearRenderTargetView(mhNormalResourceCPURTV, DirectX::Colors::AliceBlue, 0, nullptr);
	cmdList->ClearRenderTargetView(mhPositionResourceCPURTV, DirectX::Colors::AliceBlue, 0, nullptr);
	cmdList->ClearRenderTargetView(mhbaseColorAlbedoResourceCPURTV, DirectX::Colors::AliceBlue, 0, nullptr);
 	cmdList->ClearRenderTargetView(mhRoughnessMetallicAOF0ResourceCPURTV, DirectX::Colors::AliceBlue, 0, nullptr);

	cmdList->OMSetRenderTargets(5, &mRtvHeap->GetCPUDescriptorHandleForHeapStart(), true, &mDSV);
	//为了天空盒不在重复绘制
	cmdList->OMSetStencilRef(1);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mNormalResource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mPositionResource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mbaseColorAlbedoReousrce.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRoughnessMetallicAOF0.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	mcmdList->ExecuteBundle(mFirstPassBundles.Get());

	auto passCB = FrameResources->PassCB->Resource();

	auto instanceBuffer = FrameResources->InstanceBuffer->Resource();
	
	mcmdList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());

	cmdList->SetGraphicsRootShaderResourceView(1, instanceBuffer->GetGPUVirtualAddress());

	auto matBuffer = FrameResources->MaterialCB->Resource();
	cmdList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mTextureManage->GetTex2DDescHeap().Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	cmdList->SetGraphicsRootDescriptorTable(3, mTextureManage->GetTex2DDescHeap()->GetGPUDescriptorHandleForHeapStart());

	cmdList->SetPipelineState(mPSOManage->mPSOs["GbufferFirstPass"].Get());
	mRenderItemManage->Draw(cmdList, RenderLayer::Opaque, instanceBuffer);


	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mNormalResource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mPositionResource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mbaseColorAlbedoReousrce.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	mcmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRoughnessMetallicAOF0.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));


}

void Gbuffer::SecondPass(FrameResource *FrameResources,
	ID3D12GraphicsCommandList *cmdList,
	CD3DX12_CPU_DESCRIPTOR_HANDLE RenderTargetHandle,
	std::shared_ptr<ShadowMapManager> shadowMap,
	D3D12_CPU_DESCRIPTOR_HANDLE  DepthHandle,
	ID3D12Resource * RenderTarget)
{

	cmdList->RSSetViewports(1, &mViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);

	mcmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RenderTarget,
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	cmdList->OMSetRenderTargets(1, &RenderTargetHandle, true, &mDSV);

	cmdList->ClearRenderTargetView(RenderTargetHandle,DirectX::Colors::AliceBlue, 0, nullptr);
//	cmdList->ClearDepthStencilView(DepthHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);


	cmdList->SetGraphicsRootSignature(mPSOManage->mRootSignature["GbufferSecondPass"].Get());
	auto instanceBuffer = FrameResources->InstanceBuffer->Resource();

	auto passCB = FrameResources->PassCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(0, passCB->GetGPUVirtualAddress());

	cmdList->ExecuteBundle(mSecondPassBundles.Get());

	ID3D12DescriptorHeap* DepthHeaps[] = { mshadowMap->GetDescriptorHeap().Get() };
	cmdList->SetDescriptorHeaps(_countof(DepthHeaps), DepthHeaps);
	cmdList->SetGraphicsRootDescriptorTable(5, mshadowMap->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

	cmdList->SetPipelineState(mPSOManage->mPSOs["GbufferSecondPass"].Get());
	cmdList->IASetVertexBuffers(0, 1, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList->DrawInstanced(6, 1, 0, 0);

	//这里启用了一个模板测试，为的是能够提前结束对于天空盒的渲染（之前）
	cmdList->OMSetStencilRef(0);
	
	//天空盒渲染,需要注意的是这个的深度缓冲区需要使用的是First pass中的缓冲区
	cmdList->SetGraphicsRootSignature(mPSOManage->mRootSignature["Opaque"].Get());
	ID3D12DescriptorHeap* SkyCubeHeaps[] = { mTextureManage->GetCubeMapDescHeap().Get() };
	cmdList->SetDescriptorHeaps(_countof(SkyCubeHeaps), SkyCubeHeaps);
	CD3DX12_GPU_DESCRIPTOR_HANDLE skyCube(mTextureManage->GetCubeMapDescHeap()->GetGPUDescriptorHandleForHeapStart());

	cmdList->SetGraphicsRootDescriptorTable(6, skyCube);

	cmdList->SetPipelineState(mPSOManage->mPSOs["skyCube"].Get());
	mRenderItemManage->Draw(cmdList, RenderLayer::SkyCube, instanceBuffer);


	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RenderTarget,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void Gbuffer::BuildResource()
{
	//为第一次所需要的四个渲染目标创建相应的资源
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormatRGBA32F;
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
		IID_PPV_ARGS(&mNormalResource)));

	ThrowIfFailed(mdevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mPositionResource)));

	ThrowIfFailed(mdevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mbaseColorAlbedoReousrce)));

	ThrowIfFailed(mdevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mRoughnessMetallicAOF0)));
}

void Gbuffer::BuildDescriptors()
{
	//因为Gbuffer的四个缓冲，既需要作为渲染目标，同时也需要作为渲染资源，所以 描述符堆 srv 4，rtv 4+1（第二次渲染的目标）
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = 5;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(mdevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(mdevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvHeap)));
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = mFormatRGBA32F;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hRTV(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	mdevice->CreateRenderTargetView(mNormalResource.Get(), &rtvDesc, hRTV);
	hRTV.Offset(1, mRtvSize);
	mhNormalResourceCPURTV = hRTV;
	mdevice->CreateRenderTargetView(mNormalResource.Get(), &rtvDesc, hRTV);
	hRTV.Offset(1, mRtvSize);
	mhPositionResourceCPURTV = hRTV;
	mdevice->CreateRenderTargetView(mPositionResource.Get(), &rtvDesc, hRTV);
	hRTV.Offset(1, mRtvSize);
	mhbaseColorAlbedoResourceCPURTV = hRTV;
	mdevice->CreateRenderTargetView(mbaseColorAlbedoReousrce.Get(), &rtvDesc, hRTV);
	hRTV.Offset(1, mRtvSize);
	mhRoughnessMetallicAOF0ResourceCPURTV = hRTV;
	mdevice->CreateRenderTargetView(mRoughnessMetallicAOF0.Get(), &rtvDesc, hRTV);


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormatRGBA32F;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	mCbvSrvUavDescriptorSize = mdevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE hSRV(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
	mhNormalResourceCpuSRV = hSRV;
	mdevice->CreateShaderResourceView(mNormalResource.Get(), &srvDesc, hSRV);
	hSRV.Offset(1, mCbvSrvUavDescriptorSize);
	mhPositionResourceCpuSRV = hSRV;
	mdevice->CreateShaderResourceView(mPositionResource.Get(), &srvDesc, hSRV);
	hSRV.Offset(1, mCbvSrvUavDescriptorSize);
	mhbaseColorAlbedoResourceCpuSRV = hSRV;
	mdevice->CreateShaderResourceView(mbaseColorAlbedoReousrce.Get(), &srvDesc, hSRV);
	hSRV.Offset(1, mCbvSrvUavDescriptorSize);
	mhRoughnessMetallicAOF0ResourceCpuSRV = hSRV;
	mdevice->CreateShaderResourceView(mRoughnessMetallicAOF0.Get(), &srvDesc, hSRV);


	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUhandle(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	mhNormalResourceGPUSRV = GPUhandle;
	GPUhandle.Offset(1, mCbvSrvUavDescriptorSize);
	mhPositionResourceGPUSRV = GPUhandle;
	GPUhandle.Offset(1, mCbvSrvUavDescriptorSize);
	mhbaseColorAlbedoResourceGPUSRV = GPUhandle;
	GPUhandle.Offset(1, mCbvSrvUavDescriptorSize);
	mhRoughnessMetallicAOF0ResourceGPUSRV = GPUhandle;
}

void Gbuffer::BuildDepthAndStentil()
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
	depthStencilDesc.Width = mWidth;
	depthStencilDesc.Height = mHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
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
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	mdevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDSV);
// 	mdevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDSV.Offset(1,mDsvSize));
// 	mdevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDSV.Offset(1, mDsvSize));
// 	mdevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDSV.Offset(1, mDsvSize));

	// 初始化资源状态到可写的状态
	mcmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void Gbuffer::BuildBundles()
{
	ThrowIfFailed(mdevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&mFirstPsssAllocator)));
	ThrowIfFailed(mdevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&mSecondPsssAllocator)));

	ThrowIfFailed(mdevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, mFirstPsssAllocator.Get(), nullptr
		, IID_PPV_ARGS(&mFirstPassBundles)));
	ThrowIfFailed(mdevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, mSecondPsssAllocator.Get(), nullptr
		, IID_PPV_ARGS(&mSecondPassBundles)));


	//第一次缓冲未能抽出很多能够预设的 bundles
	mFirstPassBundles->SetGraphicsRootSignature(mPSOManage->mRootSignature["GbufferFirstPass"].Get());
	mFirstPassBundles->Close();

	//将第二次缓冲中一些默认堆的设置通过bundles 设置好
	mSecondPassBundles->SetGraphicsRootSignature(mPSOManage->mRootSignature["GbufferSecondPass"].Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvHeap.Get() };
	mSecondPassBundles->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mSecondPassBundles->SetGraphicsRootDescriptorTable(1, mSrvHeap->GetGPUDescriptorHandleForHeapStart());

	ID3D12DescriptorHeap* IBLProcessHeaps[] = { mIBLProcess->GetCubeMapDescHeap().Get() };
	mSecondPassBundles->SetDescriptorHeaps(_countof(IBLProcessHeaps), IBLProcessHeaps);
	CD3DX12_GPU_DESCRIPTOR_HANDLE IBLDiffuseCube(mIBLProcess->GetCubeMapDescHeap()->GetGPUDescriptorHandleForHeapStart());
	mSecondPassBundles->SetGraphicsRootDescriptorTable(2, IBLDiffuseCube);

	ID3D12DescriptorHeap* IBLSpecularHeaps[] = { mIBLSpecular->GetCubeMapDescHeap().Get() };
	mSecondPassBundles->SetDescriptorHeaps(_countof(IBLSpecularHeaps), IBLSpecularHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE IBLSpecularCube(mIBLSpecular->GetCubeMapDescHeap()->GetGPUDescriptorHandleForHeapStart());
	mSecondPassBundles->SetGraphicsRootDescriptorTable(3, IBLSpecularCube);

	ID3D12DescriptorHeap* LUTHeaps[] = { mTextureManage->GetTex2DDescHeap().Get() };
	mSecondPassBundles->SetDescriptorHeaps(_countof(LUTHeaps), LUTHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE LUTGPUHandle(mTextureManage->GetTex2DDescHeap()->GetGPUDescriptorHandleForHeapStart());
	LUTGPUHandle.Offset(mTextureManage->GetLUTOffset(), mCbvSrvUavDescriptorSize);
	mSecondPassBundles->SetGraphicsRootDescriptorTable(4, LUTGPUHandle);

	
	mSecondPassBundles->Close();

}