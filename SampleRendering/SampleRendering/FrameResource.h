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
	//��GPU������������������ص�����֮ǰ�����ǲ��ܶ����������
	//����ÿһ֡��Ҫ���Լ������������
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;
	//��GPUִ�������ô˳�������������֮ǰ�����ǲ��ܶ������и���
	//����ÿһ֡Ҫ���Լ��ĳ���������
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<UploadBuffer<MaterialData>> MaterialCB = nullptr;
	std::unique_ptr<UploadBuffer<InstanceData>> InstanceBuffer = nullptr;
	//��GPU���д�����������������֮ǰʱ���ǲ��ܶ�̬���£�����ÿһ��֡��Դ����Ҫ���Լ���WavesVB

	//ͨ��Χ��ֵ�������ǵ���Χ���㣬�����ʹ���Ǽ�⵽GPU�Ƿ�����ʹ����Щ��Դ
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
