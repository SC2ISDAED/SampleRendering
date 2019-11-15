#include "TextureManage.h"

void TextureManage::LoadTexture()
{
	for (int i = 0; i < mtextureName.size(); i++)
	{
		auto tex = std::make_unique<Texture>();
		tex->Name = mtextureName[i];
		tex->Filename = mfilePath[i];
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice, mcmdList,
			tex->Filename.c_str(), tex->Resource, tex->UploadHeap));
		mTextures[tex->Name] = std::move(tex);
	}
}

void TextureManage::BuildDescHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = Texture2DNum;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&msrvTex2D_DescHeap)));

	srvHeapDesc.NumDescriptors = Texture2DArrayNum>0?Texture2DArrayNum:1 ;//保证描述符堆里至少有一个描述符
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(msrvTex2DArray_DescHeap.GetAddressOf())));
	
	srvHeapDesc.NumDescriptors = CubeMapNum>0? CubeMapNum :1;//保证描述符堆里至少有一个描述符
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&msrvCubeMap_DescHeap)));
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//为普通二维纹理创建描述符
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(msrvTex2D_DescHeap->GetCPUDescriptorHandleForHeapStart());
	int i = 0;
	LUTHandleOffset = Texture2DNum-1;
	for (;i<Texture2DNum;i++)
	{
		//最后一个给LUT
		std::string name = mtextureName[i];
		auto tex = mTextures[name]->Resource;
		

		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = tex->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0;
		srvDesc.Texture2D.MostDetailedMip = 0;
		md3dDevice->CreateShaderResourceView(tex.Get(), &srvDesc, hDescriptor);

		hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	}

	//为二维纹理数组创建描述符
	hDescriptor= msrvTex2DArray_DescHeap->GetCPUDescriptorHandleForHeapStart();
	for (;i<Texture2DArrayNum+Texture2DNum;i++)
	{
		std::string name = mtextureName[i];
		auto tex = mTextures[name]->Resource;
		

		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Format = tex->GetDesc().Format;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = -1;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = tex->GetDesc().DepthOrArraySize;
		md3dDevice->CreateShaderResourceView(tex.Get(), &srvDesc, hDescriptor);

		hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	}
	//为立方体贴图创建描述符
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor2 (msrvCubeMap_DescHeap->GetCPUDescriptorHandleForHeapStart());
	for (; i < Texture2DArrayNum + Texture2DNum+CubeMapNum; i++)
	{
		std::string name = mtextureName[i];
		auto tex = mTextures[name]->Resource;
		

		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;

		srvDesc.TextureCube.MipLevels = tex->GetDesc().MipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		srvDesc.Format = tex->GetDesc().Format;

		md3dDevice->CreateShaderResourceView(tex.Get(), &srvDesc, hDescriptor2);

		hDescriptor2.Offset(1, mCbvSrvDescriptorSize);
	}
}
