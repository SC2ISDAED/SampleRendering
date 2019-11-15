#pragma  once
#include "Common/d3dUtil.h"
/*
 PSOManage 用来管理 PSO的整合，其中就包括了 根签名的创建 ，着色器输入的布局，着色器的编译，以及PSO的创建
*/
class PSOManage
{

public:
	PSOManage(Microsoft::WRL::ComPtr<ID3D12Device> Device,bool D4xMsaaState=false)
	{
		md3dDevice = Device;
		m4xMsaaState = D4xMsaaState;
		BuildRootSignatrue();
		BuildLayOut();
		CompleShader();
		BuildPSOs();

	}
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPSO(std::string name)
	{
		return mPSOs[name];
	}

	PSOManage(const PSOManage&rhs) = delete;
	PSOManage & operator=(const PSOManage&rhs) = delete;
	virtual ~PSOManage(){}
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>>mPSOs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>>mRootSignature;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
protected:
	void BuildRootSignatrue();
	void BuildLayOut();
	void CompleShader();
	void BuildPSOs();
private:

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();


	std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>>mInputLayout;
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice=nullptr;
	bool m4xMsaaState = false;
	UINT m4xMsaaQuality = 0;
	int msaaCount = 4;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};
