#pragma  once
#include "Common/d3dUtil.h"

class PSOManage;
class RenderItemManage;
class TextureManage;
struct  FrameResource;
class Camera;
/*
 ��������IBL������ĸ߹ⲿ�ֵĴ���ʹ�õ�����������Ⱦ��ģʽ��
 ��Ҫע����ǣ��߹ⲿ���Ǹ��ֲڶ��й�ϵ�ģ���������ʹ�õ�������6���ķּ���Դ
 ���Թ���6*6 �� ��ȾĿ��
*/
class IBLSpecular
{
public:
	IBLSpecular(ID3D12Device * device, ID3D12GraphicsCommandList *cmdList,
		std::shared_ptr<TextureManage> TextureManages,
		std::shared_ptr<PSOManage> PSOMange,
		std::shared_ptr<RenderItemManage> renderitemManage,
		FrameResource *FrameResources,
		std::vector<Camera> CubeCamera,
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
		mRtvSize = RtvDescriptorSize;
		mDsvSize = DsvDescriptorsize;
		mFrameResource = FrameResources;
		mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		mScissorRect = { 0, 0, (int)width, (int)height };
		mCamera = CubeCamera;
		BuildResource();
		BuildDescriptors();
		BuildDepthStencil();
		Process();
	}
	void OnResize(UINT newWidth, UINT newHeight);
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv();
	ID3D12Resource * Reousrce();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap >  GetCubeMapDescHeap()
	{
		return mSrvHeap;
	}
private:

	void BuildDescriptors();
	void BuildDepthStencil();
	void BuildResource();
	void UpdatePassBuffer(int level);
	void Process();
	ID3D12Device *mdevice;
	ID3D12GraphicsCommandList *mcmdList;
	//�����Ӵ����ü��������Ϣ
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
	UINT mWidth = 0;
	UINT mHeight = 0;

	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	//һ�������������棬ÿ������6��miplevel ,������Ҫ36��������
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv[36];
	CD3DX12_CPU_DESCRIPTOR_HANDLE mDSV;
	//����������Ҫ����������
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	Microsoft::WRL::ComPtr<ID3D12Resource> mRealResource;
	
	Microsoft::WRL::ComPtr<ID3D12Resource> mCubeDepthStencilBuffer;

	std::shared_ptr<PSOManage> mPSOManage;
	std::shared_ptr<RenderItemManage> mRenderItemManage;
	std::shared_ptr<TextureManage> mTextureManage;

	FrameResource * mFrameResource;
	//����ÿһ���������Ĵ�С
	UINT mRtvSize = 0;
	UINT mDsvSize = 0;
	//��Դ�ĳ���
	UINT mCubeMapSize = 512;
	std::vector<Camera> mCamera;
};