#pragma once
#include "d3dUtil.h"

template<class T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device *device, UINT elementCount, bool isConstantBuffer):mIsConstantBuffer(isConstantBuffer)
	{
		mElementByteSize = sizeof(T);

		if (isConstantBuffer)
		{
			mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));
		}
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)));
		ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mMappedData)));
		//只要还会修改数据。我们就无须取消映射
		//但是，在资源被GPU使用期间，不能向该资源进行些操作（所以必须借助于同步操作）
	}
	UploadBuffer(const UploadBuffer & rhs) = delete;
	UploadBuffer & operator =(const UploadBuffer &rhs) = delete;
	~UploadBuffer()
	{
		if (mUploadBuffer!=nullptr)
		{
			mUploadBuffer->Unmap(0, nullptr);
		}
		mMappedData = nullptr;
	}
	ID3D12Resource* Resource()const
	{
		return mUploadBuffer.Get();
	}
	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&mMappedData[elementIndex *mElementByteSize], &data, sizeof(T));
	}
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE *mMappedData = nullptr;

	UINT mElementByteSize = 0;
	bool mIsConstantBuffer = false;
};
