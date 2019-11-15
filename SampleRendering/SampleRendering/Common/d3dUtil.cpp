
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
	//创建实际的默认缓冲资源
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 
	//为了将CPU内存中的数据复制到默认缓冲区（只能由GPU访问），我们需要创建中介位置的上传堆（向此堆里的提交都需要经过cpu到gpu）
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // Describe the data we want to copy into the default buffer.
	//描述我们希望复制到默认缓冲区的数据
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Schedule to copy the data to the default buffer resource. 
	//At a high level, the helper function UpdateSubresources
	//辅助函数UpdateSubresources 将CPU中的数据复制到上传堆中，之后，使用ID3D12CommandList::CopySubresourceRegion
	//（已经细分为专门复制纹理的函数CopyTextRegion与专门复制缓冲数据的CopyBUfferRegion，在接口的源码中可以看到）
	//把中介上传堆中的数据复制到mbuffer
	//UpdateSubresources已经被修改了，与三种实现，第一种是以一块分配内存真正地实现了将资源从上传堆复制到默认堆
	//其余两种是分别先在堆或栈中分配所需内存，再调用第一种实现去执行实际地复制操作。
	//上传地数据再CD3D12X_TEXCTURE_COPY_LOCATION Dst表示地默认堆
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
	//注意，在函数调用完成后仍需保留uploadBuffer因为命令队列还可能没有执行结束真正的复制
	//需要在调用者直到复制完成后，释放uploadBuffer

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
		filename.c_str(),//pFileName希望编译的以.hlsl作为拓展名的HLSL源代码文件
		defines,		//pDefine  高级选项 参考SDK文档
		D3D_COMPILE_STANDARD_FILE_INCLUDE,//同上
		entrypoint.c_str(), //着色其入口点函数名
		target.c_str(), //pTarget指定所用着色器类型和版本的字符串
		compileFlags, //Flags1 指示着色器代码应该如何编译的比熬制
		0,
		&byteCode, //返回一个指向ID3DBlob的数据结构指针，储存这编译好的着色器对象字节
		&errors);//返回指向一个ID3DBlob 结构体的指针，发生错误时所贮存的报错字符串

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


