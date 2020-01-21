#include "TAAPass.h"
#include "FrameResource.h"
#include "PSOManage.h"

void TAAPassDraw::InilizeResource(UINT width,UINT height)
{
	

}

TAAPassDraw::TAAPassDraw(std::shared_ptr<PSOManage> PSOMange):mPSOManage(PSOMange)
{

}

void TAAPassDraw::TAAPass(ID3D12GraphicsCommandList *cmdList, FrameResource *FrameResources,
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& SrvHeap, 
	D3D12_GPU_DESCRIPTOR_HANDLE currentResourceHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE HistoryResourceHandle, ID3D12Resource * historyReousrce,
	CD3DX12_CPU_DESCRIPTOR_HANDLE RenderTargetHandle,ID3D12Resource * RenderTarget,
	D3D12_GPU_DESCRIPTOR_HANDLE UVVelocityHandle,D3D12_CPU_DESCRIPTOR_HANDLE DepthHandle)
{

	
	
	cmdList->OMSetRenderTargets(1, &RenderTargetHandle, true, &DepthHandle);

	cmdList->ClearRenderTargetView(RenderTargetHandle, DirectX::Colors::Yellow, 0, nullptr);

	cmdList->SetGraphicsRootSignature(mPSOManage->mRootSignature["TAAPass"].Get());
	auto instanceBuffer = FrameResources->InstanceBuffer->Resource();

	auto passCB = FrameResources->PassCB->Resource();

	ID3D12DescriptorHeap* descriptorHeaps[] = { SrvHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	cmdList->SetGraphicsRootDescriptorTable(0, currentResourceHandle);
	cmdList->SetGraphicsRootDescriptorTable(1, HistoryResourceHandle);
	cmdList->SetGraphicsRootDescriptorTable(2, UVVelocityHandle);

	cmdList->SetPipelineState(mPSOManage->mPSOs["TAAPass"].Get());
	cmdList->IASetVertexBuffers(0, 1, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList->DrawInstanced(6, 1, 0, 0);


	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RenderTarget,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(historyReousrce,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));

	cmdList->CopyResource(historyReousrce, RenderTarget);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(historyReousrce,
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(historyReousrce,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

}



