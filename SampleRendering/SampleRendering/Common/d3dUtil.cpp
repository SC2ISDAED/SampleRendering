
#include "d3dUtil.h"
#include <comdef.h>
#include <fstream>
#include <iostream>
using Microsoft::WRL::ComPtr;

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

bool d3dUtil::IsKeyDown(int vkeyCode)
{
    return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}

ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    // Create the actual default buffer resource.
	//����ʵ�ʵ�Ĭ�ϻ�����Դ
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 
	//Ϊ�˽�CPU�ڴ��е����ݸ��Ƶ�Ĭ�ϻ�������ֻ����GPU���ʣ���������Ҫ�����н�λ�õ��ϴ��ѣ���˶�����ύ����Ҫ����cpu��gpu��
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // Describe the data we want to copy into the default buffer.
	//��������ϣ�����Ƶ�Ĭ�ϻ�����������
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Schedule to copy the data to the default buffer resource. 
	//At a high level, the helper function UpdateSubresources
	//��������UpdateSubresources ��CPU�е����ݸ��Ƶ��ϴ����У�֮��ʹ��ID3D12CommandList::CopySubresourceRegion
	//���Ѿ�ϸ��Ϊר�Ÿ�������ĺ���CopyTextRegion��ר�Ÿ��ƻ������ݵ�CopyBUfferRegion���ڽӿڵ�Դ���п��Կ�����
	//���н��ϴ����е����ݸ��Ƶ�mbuffer
	//UpdateSubresources�Ѿ����޸��ˣ�������ʵ�֣���һ������һ������ڴ�������ʵ���˽���Դ���ϴ��Ѹ��Ƶ�Ĭ�϶�
	//���������Ƿֱ����ڶѻ�ջ�з��������ڴ棬�ٵ��õ�һ��ʵ��ȥִ��ʵ�ʵظ��Ʋ�����
	//�ϴ���������CD3D12X_TEXCTURE_COPY_LOCATION Dst��ʾ��Ĭ�϶�
    // will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
    // the intermediate upload heap data will be copied to mBuffer.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), 
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    // Note: uploadBuffer has to be kept alive after the above function calls because
    // the command list has not been executed yet that performs the actual copy.
    // The caller can Release the uploadBuffer after it knows the copy has been executed.
	//ע�⣬�ں���������ɺ����豣��uploadBuffer��Ϊ������л�����û��ִ�н��������ĸ���
	//��Ҫ�ڵ�����ֱ��������ɺ��ͷ�uploadBuffer

    return defaultBuffer;
}

ComPtr<ID3DBlob> d3dUtil::CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(
		filename.c_str(),//pFileNameϣ���������.hlsl��Ϊ��չ����HLSLԴ�����ļ�
		defines,		//pDefine  �߼�ѡ�� �ο�SDK�ĵ�
		D3D_COMPILE_STANDARD_FILE_INCLUDE,//ͬ��
		entrypoint.c_str(), //��ɫ����ڵ㺯����
		target.c_str(), //pTargetָ��������ɫ�����ͺͰ汾���ַ���
		compileFlags, //Flags1 ָʾ��ɫ������Ӧ����α���ıȰ���
		0,
		&byteCode, //����һ��ָ��ID3DBlob�����ݽṹָ�룬���������õ���ɫ�������ֽ�
		&errors);//����ָ��һ��ID3DBlob �ṹ���ָ�룬��������ʱ������ı����ַ���

	if (errors != nullptr)
	{
	//	std::cout << (char*)errors->GetBufferPointer();
		OutputDebugStringA((char*)errors->GetBufferPointer());
		
	}


	ThrowIfFailed(hr);

	return byteCode;
}

std::wstring DxException::ToString()const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}


