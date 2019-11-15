#include "PSOManage.h"

using Microsoft::WRL::ComPtr;
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> PSOManage::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy
	//专门用于阴影的比较采样器
	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);
	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp,
		shadow };
}

void PSOManage::BuildRootSignatrue()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[10];
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 26, 0, 0);
	CD3DX12_DESCRIPTOR_RANGE displacementTex;
	displacementTex.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2);
	CD3DX12_DESCRIPTOR_RANGE treeTex;
	treeTex.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 3);
	//为CubeMap创建 根签名
	CD3DX12_DESCRIPTOR_RANGE cubeTable;
	cubeTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 4);
	//创建根CBV
	CD3DX12_DESCRIPTOR_RANGE IBLDiffuseTable;
	IBLDiffuseTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 5);
	CD3DX12_DESCRIPTOR_RANGE IBLSpecularTable;
	IBLSpecularTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 6);

	CD3DX12_DESCRIPTOR_RANGE ShadowMapTable;
	ShadowMapTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 7);

	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsShaderResourceView(0, 1);
	slotRootParameter[2].InitAsShaderResourceView(1, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[4].InitAsDescriptorTable(1, &displacementTex, D3D12_SHADER_VISIBILITY_ALL);
	slotRootParameter[5].InitAsDescriptorTable(1, &treeTex, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[6].InitAsDescriptorTable(1, &cubeTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[7].InitAsDescriptorTable(1, &IBLDiffuseTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[8].InitAsDescriptorTable(1, &IBLSpecularTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[9].InitAsDescriptorTable(1, &ShadowMapTable, D3D12_SHADER_VISIBILITY_PIXEL);
	
	auto staticSamplers = GetStaticSamplers();
	
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(10, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature["Opaque"].GetAddressOf())));

	CD3DX12_ROOT_PARAMETER GbufferSlotRootParameter[4];


	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 25, 0, 0);
	GbufferSlotRootParameter[0].InitAsConstantBufferView(0);
	GbufferSlotRootParameter[1].InitAsShaderResourceView(0, 1);
	GbufferSlotRootParameter[2].InitAsShaderResourceView(1, 1);
	GbufferSlotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDescS(4, GbufferSlotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	hr = D3D12SerializeRootSignature(&rootSigDescS, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature["GbufferFirstPass"].GetAddressOf())));

	CD3DX12_ROOT_PARAMETER GbufferSecondSlotRootParameter[6];

	CD3DX12_DESCRIPTOR_RANGE Textable;
	//Normal,Position ,basecolor, roughness
	Textable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0, 0);
	CD3DX12_DESCRIPTOR_RANGE GbufferIBLDiffuseTable;
	GbufferIBLDiffuseTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0,1);
	CD3DX12_DESCRIPTOR_RANGE GbufferIBLSpecularTable;
	GbufferIBLSpecularTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0,2);
	CD3DX12_DESCRIPTOR_RANGE LUTTable;
	LUTTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 3);

	CD3DX12_DESCRIPTOR_RANGE ShadowsTable;
	ShadowsTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 4);
	GbufferSecondSlotRootParameter[0].InitAsConstantBufferView(0);
	GbufferSecondSlotRootParameter[1].InitAsDescriptorTable(1, &Textable, D3D12_SHADER_VISIBILITY_ALL);
	GbufferSecondSlotRootParameter[2].InitAsDescriptorTable(1, &GbufferIBLDiffuseTable, D3D12_SHADER_VISIBILITY_ALL);
	GbufferSecondSlotRootParameter[3].InitAsDescriptorTable(1, &GbufferIBLSpecularTable, D3D12_SHADER_VISIBILITY_ALL);
	GbufferSecondSlotRootParameter[4].InitAsDescriptorTable(1, &LUTTable, D3D12_SHADER_VISIBILITY_ALL);
	GbufferSecondSlotRootParameter[5].InitAsDescriptorTable(1, &ShadowsTable, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSecondSigDesc(6, GbufferSecondSlotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	hr = D3D12SerializeRootSignature(&rootSecondSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature["GbufferSecondPass"].GetAddressOf())));

	CD3DX12_ROOT_PARAMETER ShdowMapSlotRootParameter[4];

	ShdowMapSlotRootParameter[0].InitAsConstantBufferView(0);
	ShdowMapSlotRootParameter[1].InitAsShaderResourceView(0, 1);
	ShdowMapSlotRootParameter[2].InitAsShaderResourceView(1, 1);
	ShdowMapSlotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_ALL);


	CD3DX12_ROOT_SIGNATURE_DESC ShadowMapSigDesc(4, ShdowMapSlotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	hr = D3D12SerializeRootSignature(&ShadowMapSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);


	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature["ShadowMap"].GetAddressOf())));


	CD3DX12_ROOT_PARAMETER DebugRootParameter[2];

	CD3DX12_DESCRIPTOR_RANGE DebugTex;
	DebugTex.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0);

	DebugRootParameter[0].InitAsDescriptorTable(1, &DebugTex, D3D12_SHADER_VISIBILITY_ALL);
	DebugRootParameter[1].InitAsShaderResourceView(0, 1);

	CD3DX12_ROOT_SIGNATURE_DESC DebugSigDesc(2, DebugRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	hr = D3D12SerializeRootSignature(&DebugSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);


	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature["Debug"].GetAddressOf())));


}

void PSOManage::BuildLayOut()
{
	mInputLayout["Opaque"] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TANGENT",0, DXGI_FORMAT_R32G32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};
	mInputLayout["Irradiance"]=
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	mInputLayout["GbufferSecondPass"] =
	{
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	mInputLayout["ShadowMap"] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

	};
}

void PSOManage::CompleShader()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};
	const D3D_SHADER_MACRO waveDefines[] =
	{
		"DISPLACEMENT_MAP","1",
		NULL,NULL
	};
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");

	mShaders["skyCubeVS"] = d3dUtil::CompileShader(L"Shaders\\SkyBox.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["skyCubePS"] = d3dUtil::CompileShader(L"Shaders\\SkyBox.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["IBLIrraianceVS"] = d3dUtil::CompileShader(L"Shaders\\IrrainceIBL.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["IBLIrraiancePS"] = d3dUtil::CompileShader(L"Shaders\\IrrainceIBL.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["IBLSpecularVS"] = d3dUtil::CompileShader(L"Shaders\\Specular.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["IBLSpecularPS"] = d3dUtil::CompileShader(L"Shaders\\Specular.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["GbufferFirstPassVS"] = d3dUtil::CompileShader(L"Shaders\\GbufferFirstPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["GbufferFirstPassPS"] = d3dUtil::CompileShader(L"Shaders\\GbufferFirstPass.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["GbufferSecondPassVS"] = d3dUtil::CompileShader(L"Shaders\\GbufferSecondPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["GbufferSecondPassPS"] = d3dUtil::CompileShader(L"Shaders\\GbufferSecondPass.hlsl", nullptr, "PS", "ps_5_1");
	
	mShaders["ShadowMapVS"] = d3dUtil::CompileShader(L"Shaders\\ShadowMap.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ShadowMapPS"] = d3dUtil::CompileShader(L"Shaders\\ShadowMap.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["DebugVS"] = d3dUtil::CompileShader(L"Shaders\\Debug.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["DebugPS"] = d3dUtil::CompileShader(L"Shaders\\Debug.hlsl", nullptr, "PS", "ps_5_1");
}

void PSOManage::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout["Opaque"].data(), (UINT)mInputLayout["Opaque"].size() };
	opaquePsoDesc.pRootSignature = mRootSignature["Opaque"].Get();
	opaquePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};

	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyCubePsoDesc = opaquePsoDesc;
	skyCubePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyCubeVS"]->GetBufferPointer()),
		mShaders["skyCubeVS"]->GetBufferSize()
	};
	skyCubePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyCubePS"]->GetBufferPointer()),
		mShaders["skyCubePS"]->GetBufferSize()
	};
 	skyCubePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	skyCubePsoDesc.DepthStencilState.DepthEnable = true;
	skyCubePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
 	skyCubePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyCubePsoDesc, IID_PPV_ARGS(&mPSOs["skyCube"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC IrrianceCubePsoDesc = opaquePsoDesc;
	IrrianceCubePsoDesc.InputLayout = { mInputLayout["Irradiance"].data(), (UINT)mInputLayout["Irradiance"].size() };
	IrrianceCubePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["IBLIrraianceVS"]->GetBufferPointer()),
		mShaders["IBLIrraianceVS"]->GetBufferSize()
	};
	IrrianceCubePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["IBLIrraiancePS"]->GetBufferPointer()),
		mShaders["IBLIrraiancePS"]->GetBufferSize()
	};
	IrrianceCubePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	IrrianceCubePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&IrrianceCubePsoDesc, IID_PPV_ARGS(&mPSOs["Irradiance"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC SpecularCubePsoDesc = opaquePsoDesc;
	SpecularCubePsoDesc.InputLayout = { mInputLayout["Irradiance"].data(), (UINT)mInputLayout["Irradiance"].size() };
	SpecularCubePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["IBLSpecularVS"]->GetBufferPointer()),
		mShaders["IBLSpecularVS"]->GetBufferSize()
	};
	SpecularCubePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["IBLSpecularPS"]->GetBufferPointer()),
		mShaders["IBLSpecularPS"]->GetBufferSize()
	};
	SpecularCubePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	SpecularCubePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&SpecularCubePsoDesc, IID_PPV_ARGS(&mPSOs["IBLSpecular"])));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC GbufferFirstPassPsoDesc = opaquePsoDesc;
	GbufferFirstPassPsoDesc.InputLayout = { mInputLayout["Opaque"].data(), (UINT)mInputLayout["Opaque"].size() };
	GbufferFirstPassPsoDesc.pRootSignature = mRootSignature["GbufferFirstPass"].Get();
	GbufferFirstPassPsoDesc.DepthStencilState.DepthEnable = true;
	GbufferFirstPassPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	GbufferFirstPassPsoDesc.DepthStencilState.StencilEnable = true;
	GbufferFirstPassPsoDesc.DepthStencilState.StencilWriteMask = (UINT)0xff;
	GbufferFirstPassPsoDesc.DepthStencilState.StencilReadMask = (UINT)0xff;
	GbufferFirstPassPsoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	GbufferFirstPassPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["GbufferFirstPassVS"]->GetBufferPointer()),
		mShaders["GbufferFirstPassVS"]->GetBufferSize()
	};
	GbufferFirstPassPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["GbufferFirstPassPS"]->GetBufferPointer()),
		mShaders["GbufferFirstPassPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&GbufferFirstPassPsoDesc, IID_PPV_ARGS(&mPSOs["GbufferFirstPass"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GbufferSecondPassPsoDesc = opaquePsoDesc;
 	GbufferSecondPassPsoDesc.InputLayout = { mInputLayout["opaque"].data(), (UINT)mInputLayout["opaque"].size() };

	GbufferSecondPassPsoDesc.pRootSignature = mRootSignature["GbufferSecondPass"].Get();
	GbufferSecondPassPsoDesc.DepthStencilState.DepthEnable = true;
	GbufferSecondPassPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	GbufferSecondPassPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	GbufferSecondPassPsoDesc.DepthStencilState.StencilEnable = true;
	GbufferSecondPassPsoDesc.DepthStencilState.StencilReadMask = (UINT)0xff;
	GbufferSecondPassPsoDesc.DepthStencilState.StencilWriteMask = (UINT)0xff;
	GbufferSecondPassPsoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	GbufferSecondPassPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["GbufferSecondPassVS"]->GetBufferPointer()),
		mShaders["GbufferSecondPassVS"]->GetBufferSize()
	};
	GbufferSecondPassPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["GbufferSecondPassPS"]->GetBufferPointer()),
		mShaders["GbufferSecondPassPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&GbufferSecondPassPsoDesc, IID_PPV_ARGS(&mPSOs["GbufferSecondPass"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = opaquePsoDesc;
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	smapPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	smapPsoDesc.pRootSignature = mRootSignature["ShadowMap"].Get();
	smapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ShadowMapVS"]->GetBufferPointer()),
		mShaders["ShadowMapVS"]->GetBufferSize()
	};
	smapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ShadowMapPS"]->GetBufferPointer()),
		mShaders["ShadowMapPS"]->GetBufferSize()
	};

	// Shadow map pass does not have a render target.
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs["ShadowMap"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = opaquePsoDesc;
/* 	debugPsoDesc.InputLayout = { mInputLayout["ShadowMap"].data(), (UINT)mInputLayout["ShadowMap"].size() };*/
	debugPsoDesc.DepthStencilState.DepthEnable = true;
	debugPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	debugPsoDesc.DepthStencilState.StencilEnable = true;

 	debugPsoDesc.pRootSignature = mRootSignature["Debug"].Get();
	debugPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["DebugVS"]->GetBufferPointer()),
		mShaders["DebugVS"]->GetBufferSize()
	};
	debugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["DebugPS"]->GetBufferPointer()),
		mShaders["DebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&mPSOs["Debug"])));

}
