#include "Common/d3dUtil.h"
struct Light;
struct FrameResource;
class PSOManage;
class RenderItemManage;
class Camera;
/*
 最简单的阴影贴图，甚至连多光源都还没有进行测试，只做了单个直接光源
 进行阴影的Pass ，需要将摄像机转换到光源方向，并向场景中投射，生成阴影贴图。
*/
class ShadowMapManager
{
public:
	ShadowMapManager(ID3D12Device* device,
		UINT width, UINT height,
		std::vector<Light>lightArray,
		std::shared_ptr<PSOManage> PSOMange,
		std::shared_ptr<RenderItemManage> renderitemManage,
		UINT SRVDescriptorSize,
		UINT DsvDescriptorsize) :

		mlightArray(std::move(lightArray))
	{
		mdevice = device;
		mWidth = width;
		mHeight = height;

		mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		mScissorRect = { 0, 0, (int)width, (int)height };

		ShadowMapNumber = mlightArray.size();

		mSrvSize = SRVDescriptorSize;
		mDsvSize = DsvDescriptorsize;


		mPSOManage = PSOMange;
		mRenderItemManage = renderitemManage;

		BuildResource();
		BuildDescriptors();
		CreateCommand();
	};

	ShadowMapManager(const ShadowMapManager& rhs) = delete;
	ShadowMapManager& operator=(const ShadowMapManager& rhs) = delete;
	~ShadowMapManager() = default;

	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv()const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE Dsv()const;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap > GetDescriptorHeap();
	D3D12_VIEWPORT Viewport()const;
	D3D12_RECT ScissorRect()const;


	void OnResize(UINT newWidth, UINT newHeight);
	void Draw(ID3D12GraphicsCommandList *cmdList,
		FrameResource *CurrentFrameResource,
		DirectX::BoundingSphere *mSceneBounds, Camera * camera);
private:
	void BuildDescriptors();
	void BuildResource();
	void UpdateMainPass(int index, FrameResource *CurrentFrameResource, DirectX::BoundingSphere *mSceneBounds);
	void CreateCommand();
private:

	ID3D12Device* mdevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;

	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> mhCpuSrv;
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> mhGpuSrv;
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> mhCpuDsv;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	std::vector < Microsoft::WRL::ComPtr<ID3D12Resource>> mShadowMap;

	std::shared_ptr<PSOManage> mPSOManage;
	std::shared_ptr<RenderItemManage> mRenderItemManage;

	std::vector<Light> mlightArray;

	UINT mSrvSize = 0;
	UINT mDsvSize = 0;


	Camera * mcamera;
	int ShadowMapNumber = 0;


	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
};