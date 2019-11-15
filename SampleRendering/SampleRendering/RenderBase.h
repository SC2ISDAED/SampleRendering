#pragma once
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "BaseStruct.h"
#include <DirectXMath.h>
#include "GameTimer.h"
#include "Common/d3dUtil.h"
#include "D3d12SDKLayers.h"
#include "Common/Camera.h"
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")

class PSOManage;
class TextureManage;
class RenderItemManage;
struct FrameResource;
class IBLPorcess;
class IBLSpecular;
class Gbuffer;
class ShadowMapManager;

enum class Thread:int
{
	One,
	Two,
	Three,
	Four,
	Count
}; 

class RenderBase
{
public:
	RenderBase(int clientWidth = 800, int clientHeight = 600)
	{
		mClientHeight = clientHeight;
		mClientWidth = mClientWidth;
	}
	RenderBase(const RenderBase & rhs) = delete;
	RenderBase & operator =(const RenderBase& rhs) = delete;
	virtual ~RenderBase();
public:
	bool initDirect3D();

	void SetMSAA()
	{
		m4xMsaaState = !m4xMsaaState;
		OnResize(mClientHeight,mClientHeight);
	};

	bool  DeviceIsNull()
	{
		return (md3dDevice==nullptr);
	}
	//调整窗口属性时需要调用改此函数，重置缓冲区
	void OnResize(int clientWidth,int clientHeight);

public:
	//创建命令对象
	void CreateCommandObjects();
	//创建双缓冲交换链
	void CreateSwapChain();
	//创建深度缓冲
	void CreateDepth();
	//创建多重采样相关
	void CreateMSAA();
	//创建每一帧所需资源
	void CreateFrameResource();

	void FlushCommandQueue();

	UINT BuildLight();
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;

	Camera mCamera;
	std:: vector<Camera> mCubeMapCamera;
	void BuildCubeFaceCamera();

	//创建RTV和DSV描述符
	void CreateRtvAndDsvDescriptorHeaps();

	void LoadTextureAndCreateDescriptroHeaps();

	
	//更新相关
	void Update(GameTimer& gt);
	void UpdatePassCB(GameTimer& gt);
	//绘制相关
	void Draw(GameTimer& gt);
	void ForwardRendering();
	//鼠标键盘事件相应
	void OnMouseUp(int x, int y, POINT mLastMousePox);
	void OnMouseDown(int x, int y, POINT mLastMousePox);
	void OnMouseMove(int x, int y, POINT mLastMousePox);
	void OnKeyboardInput(GameTimer& gt);

	void SetMainWnd(HWND hMainWnd)
	{
		mhMainWnd = hMainWnd;
	};
	void SetDebugRendering()
	{
		BDebugRendering = !BDebugRendering;
	}
protected:
	void CreateDevice();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	bool m4xMsaaState = false;
	int msaaCount = 4;
	UINT m4xMsaaQuality = 0;
	bool BDebugRendering = false;

	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	Microsoft::WRL::ComPtr<ID3D12Fence> mCopyFence;
	Microsoft::WRL::ComPtr<ID3D12Fence> mComputeFence;
	UINT64 mCurrentFence = 0;
	UINT64 mCopyFenceVal = 0;
	UINT64 mComputeFenceVal = 0;
	//命令队列，命令分配器，以及命令列表
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mDirectQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mComputeQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCopyQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mTextureCmdListAlloc;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mComputeCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCopyCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mTextureCommandList;
\
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mComputeList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCopyList;

	//创建交换链，也就是实现双缓冲或者更高的缓冲
	static const int SwapChainBufferCount = 2;
	int mCurrentBackBuffer = 0;
	
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> m_msaaRenderTarget;
	Microsoft::WRL::ComPtr<ID3D12Resource>  mDepthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_msaaRTVDescriptor;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_msaaSRVDescriptor;

	D3D12_VIEWPORT mMainScreenViewport;
	D3D12_RECT mMainScissorRect;

	D3D12_VIEWPORT mDebugScreenViewport[4];
	int DebugWidth = 200;
	int DebugHeight = 150;
	D3D12_RECT mDebugScissorRect[4];

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;
	//用户应该在派生类的派生构造函数中自定义这些初始值
	std::wstring mMainWndCaption = L"d3d App";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	std::shared_ptr<PSOManage> mPSOManage;
	std::shared_ptr<TextureManage> mTextureManage;
	std::shared_ptr<RenderItemManage> mRenderItemManage;
	std::shared_ptr<IBLPorcess> mIBLProcess;
	std::shared_ptr<IBLSpecular> mIBLSpecular;
	std::shared_ptr<Gbuffer> mGbuffer;
	std::shared_ptr<ShadowMapManager> mShadowMapManager;
	std::vector<std::shared_ptr<FrameResource>>mFrameResources;

	std::vector<Light> mlightArray;
	int mCurrentFrameResourceIndex = 0;
	FrameResource *mCurrentFrameResource = nullptr;

	PassConstants mMainPassCB;
	int mClientWidth = 1000;
	int mClientHeight = 750;
	HWND mhMainWnd = nullptr;

	DirectX::BoundingSphere mSceneBounds;

	float mSunTheta = 1.25f* DirectX::XM_PI;
	float mSunPhi = DirectX::XM_PIDIV4;


	HRESULT Result;

	const int  CubeMapSize = 512;

	friend UINT  BuildPSOThread(void* arg);
	friend UINT   BuildRenderItemThread(void* arg);
	friend UINT  BuildTextureManagerThread(void* arg);
};
