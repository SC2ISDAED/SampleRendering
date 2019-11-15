#pragma once
#include "Common/d3dUtil.h"
/*
 使用这个类进行Texture的管理
 目前将tex分为了3类，Tex，TexArray，CubeMap
 需要改进的是利用了DirectX::CreateDDSTextureFromFile12()这个辅助函数进行读取纹理
 将纹理格式限制正在了.dds，并且隐藏了未能注意到重要的细节
*/
class TextureManage
{
public:
	/*
	需要写入的是纹理的名字，及其对应的路径名，将纹理分为了3种，分别是texture2d textureArray，CubeMap 需要分别写入填入各自的数目
	@program Texture2DNumber 2维纹理的数目
	@program TextureArrayNumber 纹理数组的数目
	@program CubeMapNumber    立方体贴图的数目
	*/
	TextureManage(ID3D12Device* Device,
		ID3D12GraphicsCommandList* cmdList, 
    std::vector<std::string> &textureName,
	std::vector<std::wstring> &filePath,
	int Texture2DNumber,
	int TextureArrayNumber,
	int CubeMapNumber,
	UINT CbvSrvDescriptorSize
	)
	{
		md3dDevice = Device;
		mcmdList = cmdList;
		mtextureName = textureName;
		mfilePath =filePath;
		Texture2DNum = Texture2DNumber;
		Texture2DArrayNum = TextureArrayNumber;
		CubeMapNum = CubeMapNumber;
		mCbvSrvDescriptorSize = CbvSrvDescriptorSize;
		LoadTexture();
		BuildDescHeap();
	}
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap > GetTex2DDescHeap()
	{
		return msrvTex2D_DescHeap;
	}
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap > GetTexArray2DDescHeap()
	{
		return msrvTex2DArray_DescHeap;
	}
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap >  GetCubeMapDescHeap()
	{
		return msrvCubeMap_DescHeap;
	}
	UINT GetLUTOffset()
	{
		return LUTHandleOffset;
	}
	~TextureManage()
	{

	}
private:
	void LoadTexture();
	void BuildDescHeap();
	int Texture2DNum = 0;
	int Texture2DArrayNum = 0;
	int CubeMapNum = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap >msrvTex2D_DescHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap >msrvTex2DArray_DescHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap >msrvCubeMap_DescHeap = nullptr;
	UINT mCbvSrvDescriptorSize = 0;
	UINT LUTHandleOffset;
	std::vector<std::string > mtextureName;
	std::vector<std::wstring> mfilePath;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	ID3D12GraphicsCommandList* mcmdList;
	ID3D12Device* md3dDevice;
};