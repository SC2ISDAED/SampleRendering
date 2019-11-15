#pragma once
#include "Common/d3dUtil.h"
/*
������Gbuffer�࣬ͨ������ Rendering pass,
��һ�ν� Position ,Normal��basecolor��Alboe��Roughnes.Metallic.AO.F0�ֱ������4����ȾĿ���н�����Ⱦ����û�н��������Ż�
�ڶ���Rendering pass ���ݵ�һ�� ����Ϣ�����Ϲ�Դ������ ���й���ɫ
*/

class PSOManage;
class RenderItemManage;
class TextureManage;
class IBLPorcess;
class IBLSpecular;
class ShadowMapManager;
struct  FrameResource;
class Gbuffer
{
public:

	Gbuffer(ID3D12Device * device, ID3D12GraphicsCommandList *cmdList,
		std::shared_ptr<TextureManage> TextureManages,
		std::shared_ptr<PSOManage> PSOMange,
		std::shared_ptr<RenderItemManage> renderitemManage,
		std::shared_ptr<ShadowMapManager> ShadowMap,
		std::shared_ptr<IBLSpecular> pIBLSpecular,std::shared_ptr<IBLPorcess> pIBLProcess,
		UINT width, UINT height,
		UINT RtvDescriptorSize,
		UINT DsvDescriptorsize)
	{
		mdevice = device;
		mcmdList = cmdList;

		mWidth = width;
		mHeight = height;

		mPSOManage = PSOMange;
		mRenderItemManage = renderitemManage;
		mTextureManage = TextureManages;

		mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		mScissorRect = { 0, 0, (int)width, (int)height };

		mRtvSize = RtvDescriptorSize;
		mDsvSize = DsvDescriptorsize;

		mIBLProcess = pIBLProcess;
		mIBLSpecular = pIBLSpecular;

		mshadowMap = ShadowMap;
		BuildResource();
		BuildDepthAndStentil();
		BuildDescriptors();
		BuildBundles();
	}
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDescHeap();
	CD3DX12_GPU_DESCRIPTOR_HANDLE NormalSrv();
	CD3DX12_GPU_DESCRIPTOR_HANDLE PositionSrv();
	CD3DX12_GPU_DESCRIPTOR_HANDLE baseColorAlbedoSrv();
	CD3DX12_GPU_DESCRIPTOR_HANDLE RoughnessMetallicAOF0Srv();
	void FirstPass(FrameResource *FrameResources, ID3D12GraphicsCommandList *cmdList);
	void SecondPass(FrameResource *FrameResources,
		ID3D12GraphicsCommandList *cmdList,
		CD3DX12_CPU_DESCRIPTOR_HANDLE RenderTargetHandle,
		std::shared_ptr<ShadowMapManager> shadowMap,
		D3D12_CPU_DESCRIPTOR_HANDLE  DepthHandle,
		ID3D12Resource * RenderTarget);
public:
	void BuildResource();
	void BuildDescriptors();
	void BuildDepthAndStentil();
	void BuildBundles();
	Microsoft::WRL::ComPtr<ID3D12Resource> mNormalResource;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalResourceCpuSRV;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhNormalResourceGPUSRV;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalResourceCPURTV;

	Microsoft::WRL::ComPtr<ID3D12Resource> mPositionResource;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhPositionResourceCpuSRV;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhPositionResourceGPUSRV;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhPositionResourceCPURTV;

	Microsoft::WRL::ComPtr<ID3D12Resource> mbaseColorAlbedoReousrce;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhbaseColorAlbedoResourceCpuSRV;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhbaseColorAlbedoResourceGPUSRV;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhbaseColorAlbedoResourceCPURTV;

	Microsoft::WRL::ComPtr<ID3D12Resource> mRoughnessMetallicAOF0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhRoughnessMetallicAOF0ResourceCpuSRV;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhRoughnessMetallicAOF0ResourceGPUSRV;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhRoughnessMetallicAOF0ResourceCPURTV;

	
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mDSV;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;


	DXGI_FORMAT mFormatRGB32F = DXGI_FORMAT_R32G32B32_FLOAT;
	DXGI_FORMAT mFormatRGBA32F = DXGI_FORMAT_R32G32B32A32_FLOAT;
	DXGI_FORMAT mFormatRGBA8F = DXGI_FORMAT_R8G8B8A8_UNORM;

	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	ID3D12Device *mdevice;
	ID3D12GraphicsCommandList *mcmdList;

	//����ʹ��bundles ��Ҫ��Ϊ�˼�����ÿһ֡��Ⱦ��ʱ��
	//�����ظ�ʹ�� commandlist ȥ��¼���
	//����ͨ��BundlesԤ�ȼ�¼һЩ��������ӵ�commandlist��
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mFirstPassBundles;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mSecondPassBundles;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator > mFirstPsssAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator > mSecondPsssAllocator;
	std::shared_ptr<PSOManage> mPSOManage;
	std::shared_ptr<RenderItemManage> mRenderItemManage;
	std::shared_ptr<TextureManage> mTextureManage;
	std::shared_ptr<ShadowMapManager> mshadowMap;

	std::shared_ptr<IBLPorcess>mIBLProcess;
	std::shared_ptr<IBLSpecular>mIBLSpecular;

	FrameResource * mFrameResource;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvSize = 0;
	UINT mDsvSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;
	UINT mWidth = 0;
	UINT mHeight = 0;

};