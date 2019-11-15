#include "Common/d3dUtil.h"
class PSOManage;
class RenderItemManage;
class TextureManage;
struct  FrameResource;
enum class CubeMapFace : int
{
	PositiveX = 0,
	NegativeX = 1,
	PositiveY = 2,
	NegativeY = 3,
	PositiveZ = 4,
	NegativeZ = 5
};
/*
 ����ûȡ׼���������IBL��Ԥ��Ⱦ����һ���࣬
 �������Ҫ�ǽ���IBL�����������䲿�ֵĴ������õ�����������ͼ�ķ�ʽ������Ⱦ������
 Ŀǰ��Ҫ���ڵ������ǣ���Ⱦ�������mianpass ������Ҫ�ֱ�����ύ��������һ�����⣬
 ���Ƿ�������ʵʱ�޸��ύһ��mianpass����һ������������ֱ����
*/
class IBLPorcess
{
public:
	IBLPorcess(ID3D12Device * device, ID3D12GraphicsCommandList *cmdList,
		std::shared_ptr<TextureManage> TextureManages,
		std::shared_ptr<PSOManage> PSOMange,
		std::shared_ptr<RenderItemManage> renderitemManage,
		FrameResource *FrameResources,
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

	void Process();
	ID3D12Device *mdevice;
	ID3D12GraphicsCommandList *mcmdList;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
	UINT mWidth = 0;
	UINT mHeight = 0;

	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv[6];
	CD3DX12_CPU_DESCRIPTOR_HANDLE mDSV;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	Microsoft::WRL::ComPtr<ID3D12Resource> mRealResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> mCubeDepthStencilBuffer;

	std::shared_ptr<PSOManage> mPSOManage;
	std::shared_ptr<RenderItemManage> mRenderItemManage;
	std::shared_ptr<TextureManage> mTextureManage;

	FrameResource * mFrameResource;

	UINT mRtvSize = 0;
	UINT mDsvSize = 0;

	UINT mCubeMapSize = 512;
};