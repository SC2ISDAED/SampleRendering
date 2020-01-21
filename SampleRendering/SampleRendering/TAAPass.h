#pragma once
#include "Common/d3dUtil.h"
class PSOManage;
class FrameResource;
class TAAPassDraw
{
private:
	void InilizeResource(UINT width, UINT height);
public:
	TAAPassDraw(std::shared_ptr<PSOManage> PSOMange);
	
	void TAAPass(ID3D12GraphicsCommandList *cmdList, FrameResource *FrameResources,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &mSrvHeap,
		D3D12_GPU_DESCRIPTOR_HANDLE currentResourceHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE HistoryResourceHandle, ID3D12Resource * historyReousrce,
		CD3DX12_CPU_DESCRIPTOR_HANDLE RenderTargetHandle, ID3D12Resource * RenderTarget, 
		D3D12_GPU_DESCRIPTOR_HANDLE UVVelocityHandle,D3D12_CPU_DESCRIPTOR_HANDLE DepthHandle);

	ID3D12Device *mdevice;
	std::shared_ptr<PSOManage> mPSOManage;

};