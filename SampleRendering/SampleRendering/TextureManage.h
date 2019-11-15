#pragma once
#include "Common/d3dUtil.h"
/*
 ʹ����������Texture�Ĺ���
 Ŀǰ��tex��Ϊ��3�࣬Tex��TexArray��CubeMap
 ��Ҫ�Ľ�����������DirectX::CreateDDSTextureFromFile12()��������������ж�ȡ����
 �������ʽ����������.dds������������δ��ע�⵽��Ҫ��ϸ��
*/
class TextureManage
{
public:
	/*
	��Ҫд�������������֣������Ӧ��·�������������Ϊ��3�֣��ֱ���texture2d textureArray��CubeMap ��Ҫ�ֱ�д��������Ե���Ŀ
	@program Texture2DNumber 2ά�������Ŀ
	@program TextureArrayNumber �����������Ŀ
	@program CubeMapNumber    ��������ͼ����Ŀ
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