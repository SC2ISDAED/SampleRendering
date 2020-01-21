#pragma once
#include "BaseStruct.h"
#include "Common/UploadBuffer.h"
#include "Common/Camera.h"
class GameTimer;
struct RenderItem;

struct FrameResource 
{
public:
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount,
		UINT MaterialCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource & operator=(const FrameResource&rhs) = delete;
	~FrameResource() {};
	//在GPU处理完此命令分配器相关的命令之前，我们不能对其进行重置
	//所以每一帧都要由自己的命令分配器
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;
	//在GPU执行完引用此常量缓冲区命令之前，我们不能对它进行更新
	//所以每一帧要有自己的常量缓冲区
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<UploadBuffer<MaterialData>> MaterialCB = nullptr;
	std::unique_ptr<UploadBuffer<InstanceData>> InstanceBuffer = nullptr;
	//在GPU进行处理完引用它的命令之前时我们不能动态更新，所以每一个帧资源都需要由自己的WavesVB

	//通过围栏值将命令标记到此围栏点，这可以使我们检测到GPU是否仍在使用这些资源
	UINT64 Fence = 0;
	void UpdateFrameResource(const GameTimer &gt,
		const std::vector<std::unique_ptr<RenderItem>> &mAllRitems,
		PassConstants &mMainPassCB,
		const std::unordered_map<std::string,
		std::unique_ptr<Material>> & mMaterials,
		const Camera &mCamera);
	void UpdateInstancedData(const GameTimer &gt, const std::vector<std::unique_ptr<RenderItem>> &mAllRitems,
		const Camera& mCamera);
	void InlizeInstancedData(const std::vector<std::unique_ptr<RenderItem>> &mAllRitems,
		const Camera& mCamera);
	void UpdateMainPassCBS(PassConstants &mMainPassCB);
private:



	void UpdateMaterialCBS(const GameTimer &gt,const std::unordered_map<std::string, std::unique_ptr<Material>> & mMaterials);

	void BuildCubeMapCamera(PassConstants * cubeFacePassCB);
};
