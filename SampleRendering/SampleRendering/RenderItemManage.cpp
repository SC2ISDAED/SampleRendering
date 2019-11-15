#include "RenderItemManage.h"
#include "Common/MathHelper.h"
#include "BaseStruct.h"
using namespace DirectX;

float GetHillsHeight(float x, float z)
{
	return 0.3f*(z*sinf(0.1f*x) + x * cosf(0.1f*z));
}

DirectX::XMFLOAT3 GetHillsNormal(float x, float z)
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

/*
 使用这个函数进行绘制命令的记录，主要还是能直接设置 顶点，顶点索引的数据，并且使用了实例化的渲染方式
*/
 void RenderItemManage::Draw(ID3D12GraphicsCommandList *cmdList, RenderLayer layer, ID3D12Resource* InsatanceBuffer)
{
	const std::vector<RenderItem *>& ritems= mRenderItemLayer[(int)RenderLayer(layer)];
	for (size_t i=0;i<ritems.size();i++)
	{

	

		UINT objCBByteSize = sizeof(InstanceData);
		auto ri = ritems[i];

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = InsatanceBuffer->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		cmdList->SetGraphicsRootShaderResourceView(1, objCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount,ri->Instances.size() , ri->StartIndexLocation, ri->BaseVertexLocation, 0);

	}
}

void RenderItemManage::BuildGeomery(ID3D12Device* Device, ID3D12GraphicsCommandList * cmdList)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
	GeometryGenerator::MeshData quad = geoGen.CreateQuad(-1.0f, -0.5f, 0.5f, 0.5f, 0.5f);
	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
	UINT quadVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();
	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
	UINT quadIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();
	// Define the SubmeshGeometry that cover different 
	// regions of the vertex/index buffers.

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	SubmeshGeometry quadSubmesh;
	quadSubmesh.IndexCount = (UINT)quad.Indices32.size();
	quadSubmesh.StartIndexLocation = quadIndexOffset;
	quadSubmesh.BaseVertexLocation = quadVertexOffset;
	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size()+
		quad.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TangentU = box.Vertices[i].TangentU;
		vertices[k].TexC = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TangentU = grid.Vertices[i].TangentU;
		vertices[k].TexC = grid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TangentU = sphere.Vertices[i].TangentU;
		vertices[k].TexC = sphere.Vertices[i].TexC;

	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TangentU = cylinder.Vertices[i].TangentU;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
	}
	for (int i = 0; i < quad.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = quad.Vertices[i].Position;
		vertices[k].Normal = quad.Vertices[i].Normal;
		vertices[k].TangentU = quad.Vertices[i].TangentU;
		vertices[k].TexC = quad.Vertices[i].TexC;
	}
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(quad.GetIndices16()), std::end(quad.GetIndices16()));
	
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(Device,
	cmdList, vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(Device,
		cmdList, indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;
	geo->DrawArgs["quad"] = quadSubmesh;

	mGeometries[geo->Name] = std::move(geo);
	
}

void RenderItemManage::BuildBoxGeometry(ID3D12Device* Device, ID3D12GraphicsCommandList * cmdList)
{
	struct Vertex
	{
		XMFLOAT3 Pos;
		XMFLOAT4 Color;
	};
	std::array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(Device,
		cmdList, vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(Device,
		cmdList, indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(Vertex);
	mBoxGeo->VertexBufferByteSize = vbByteSize;
	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;
}
/*
  进行材质的创建，目前每一种材质是需要 五个贴图，并且由于实际的贴图数据是一块提交的，所以需要记录相应的偏移索引Offset
*/
void RenderItemManage::BuildMaterial()
{
	int Offset = 0;
	auto StreakedMaterial = std::make_unique<Material>();
	StreakedMaterial->MatCBIndex = 0;
	StreakedMaterial->Name = "StreakedMat";
	StreakedMaterial->AlbeoSrvHeapIndex = 0;
	StreakedMaterial->NormalSrvHeapIndex = 1;
	StreakedMaterial->MetalSrvHeapIndex = 2;
	StreakedMaterial->RoughnessSrvHeapIndex = 3;
	StreakedMaterial->AOMapSrvHeapIndex = 4;
	mMaterials[StreakedMaterial->Name] = std::move(StreakedMaterial);
	Offset += 5;
	auto BambooMaterial = std::make_unique<Material>();
	BambooMaterial->MatCBIndex = 1;
	BambooMaterial->Name = "BambooMat";
	BambooMaterial->AlbeoSrvHeapIndex = 0+ Offset;
	BambooMaterial->NormalSrvHeapIndex = 1+ Offset;
	BambooMaterial->MetalSrvHeapIndex = 2+ Offset;
	BambooMaterial->RoughnessSrvHeapIndex = 3+ Offset;
	BambooMaterial->AOMapSrvHeapIndex = 4+ Offset;
	Offset += 5;
	mMaterials[BambooMaterial->Name] = std::move(BambooMaterial);

	auto greasyMaterial = std::make_unique<Material>();
	greasyMaterial->MatCBIndex = 2;
	greasyMaterial->Name = "greasyMat";
	greasyMaterial->AlbeoSrvHeapIndex = 0 + Offset;
	greasyMaterial->NormalSrvHeapIndex = 1 + Offset;
	greasyMaterial->MetalSrvHeapIndex = 2 + Offset;
	greasyMaterial->RoughnessSrvHeapIndex = 3 + Offset;
	greasyMaterial->AOMapSrvHeapIndex = 4 + Offset;
	Offset += 5;
	mMaterials[greasyMaterial->Name] = std::move(greasyMaterial);

	auto rustMaterial = std::make_unique<Material>();
	rustMaterial->MatCBIndex = 3;
	rustMaterial->Name = "rustMat";
	rustMaterial->AlbeoSrvHeapIndex = 0+ Offset;
	rustMaterial->NormalSrvHeapIndex = 1+ Offset;
	rustMaterial->MetalSrvHeapIndex = 2+ Offset;
	rustMaterial->RoughnessSrvHeapIndex = 3+ Offset;
	rustMaterial->AOMapSrvHeapIndex = 4+ Offset;
	Offset += 5;
	mMaterials[rustMaterial->Name] = std::move(rustMaterial);

	auto mahogfloorMaterial = std::make_unique<Material>();
	mahogfloorMaterial->MatCBIndex = 4;
	mahogfloorMaterial->Name = "rustmahogfloorMat";
	mahogfloorMaterial->AlbeoSrvHeapIndex = 0 + Offset;
	mahogfloorMaterial->NormalSrvHeapIndex = 1 + Offset;
	mahogfloorMaterial->MetalSrvHeapIndex = 2 + Offset;
	mahogfloorMaterial->RoughnessSrvHeapIndex = 3 + Offset;
	mahogfloorMaterial->AOMapSrvHeapIndex = 4 + Offset;
	Offset += 5;
	mMaterials[mahogfloorMaterial->Name] = std::move(mahogfloorMaterial);

}
/*
这个函数是目前主要用到进行创建RenderItem，RenderItem 包含了所需的顶点索引信息，模型的材质
目前的主要问题是，进行实例化渲染，而实例化渲染是通了Frameresource 进行统一的提交，所以还需要记录在实例化数据中的索引，Instanceindex
并且我目前认为这里多线程应该能发挥大作用
*/
void RenderItemManage::BuildRenderItem()
{

	int InstanceIndex = 0;
	auto boxRitem = std::make_unique<RenderItem>();
	boxRitem->ObjCBIndex = InstanceIndex;
	boxRitem->Geo = mGeometries["shapeGeo"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	boxRitem->Instances.resize(2);
	XMStoreFloat4x4(&boxRitem->Instances[0].World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	boxRitem->Instances[0].MaterialIndex = 3;
	boxRitem->Instances[0].TexTransform;
	XMStoreFloat4x4(&boxRitem->Instances[0].TexTransform, XMMatrixScaling(1.0f, 10.0f, 1.0f));


	XMStoreFloat4x4(&boxRitem->Instances[1].World, XMMatrixTranslation(10.0f, 15.0f, 10.0f));
	boxRitem->Instances[1].MaterialIndex = mMaterials["StreakedMat"]->MatCBIndex;
	boxRitem->Instances[1].TexTransform;
	XMStoreFloat4x4(&boxRitem->Instances[1].World, XMMatrixTranslation(1.0f, 1.0f, 1.0f));

// 	XMStoreFloat4x4(&boxRitem->Instances[2].World, XMMatrixTranslation(1.0f, 1.0f, 1.0f));
// 	boxRitem->Instances[2].MaterialIndex = mMaterials["StreakedMat"]->MatCBIndex;
// 	boxRitem->Instances[2].TexTransform;
// 	XMStoreFloat4x4(&boxRitem->Instances[2].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));

	InstanceIndex += boxRitem->Instances.size();
	mAllRitems.push_back(std::move(boxRitem));

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = InstanceIndex;
	gridRitem->Geo = mGeometries["shapeGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	gridRitem->Instances.resize(1);
	XMStoreFloat4x4(&gridRitem->Instances[0].World, XMMatrixScaling(2.0f, 2.0f, 2.0f));
	XMStoreFloat4x4(&gridRitem->Instances[0].TexTransform, XMMatrixScaling(5.0f, 5.0f, 5.0f));
	gridRitem->Instances[0].MaterialIndex = mMaterials["StreakedMat"]->MatCBIndex;
	gridRitem->Instances[0].TexTransform;
	InstanceIndex += gridRitem->Instances.size();
	mAllRitems.push_back(std::move(gridRitem));



	for (int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<RenderItem>();
		auto rightCylRitem = std::make_unique<RenderItem>();
		auto leftSphereRitem = std::make_unique<RenderItem>();
		auto rightSphereRitem = std::make_unique<RenderItem>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

		XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
		leftCylRitem->ObjCBIndex = InstanceIndex++;
		leftCylRitem->Geo = mGeometries["shapeGeo"].get();
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
		leftCylRitem->Instances.resize(1);
		leftCylRitem->Instances[0].MaterialIndex = i;
		XMStoreFloat4x4(&leftCylRitem->Instances[0].World, leftCylWorld);
		XMStoreFloat4x4(&leftCylRitem->Instances[0].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));

		XMStoreFloat4x4(&rightCylRitem->World, rightCylWorld);
		rightCylRitem->ObjCBIndex = InstanceIndex++;
		rightCylRitem->Geo = mGeometries["shapeGeo"].get();
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
		rightCylRitem->Instances.resize(1);
		XMStoreFloat4x4(&rightCylRitem->Instances[0].World, rightCylWorld);
		rightCylRitem->Instances[0].MaterialIndex = i;
		XMStoreFloat4x4(&rightCylRitem->Instances[0].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));

		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->ObjCBIndex = InstanceIndex++;
		leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
		leftSphereRitem->Instances.resize(1);
		XMStoreFloat4x4(&leftSphereRitem->Instances[0].World, leftSphereWorld);
		leftSphereRitem->Instances[0].MaterialIndex = i;

		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->ObjCBIndex = InstanceIndex++;
		rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
		rightSphereRitem->Instances.resize(1);
		XMStoreFloat4x4(&rightSphereRitem->Instances[0].World, rightSphereWorld);
		rightSphereRitem->Instances[0].MaterialIndex = i;
// 
		mAllRitems.push_back(std::move(leftCylRitem));
		mAllRitems.push_back(std::move(rightCylRitem));
		mAllRitems.push_back(std::move(leftSphereRitem));
		mAllRitems.push_back(std::move(rightSphereRitem));
	}
	// All the render items are opaque.
	for (auto& e : mAllRitems)
		mRenderItemLayer[(int)RenderLayer::Opaque].push_back(e.get());


	auto quadRitem = std::make_unique<RenderItem>();
	quadRitem->ObjCBIndex = InstanceIndex;
	quadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	quadRitem->Geo = mGeometries["shapeGeo"].get();
	quadRitem->IndexCount = quadRitem->Geo->DrawArgs["quad"].IndexCount;
	quadRitem->StartIndexLocation = quadRitem->Geo->DrawArgs["quad"].StartIndexLocation;
	quadRitem->BaseVertexLocation = quadRitem->Geo->DrawArgs["quad"].BaseVertexLocation;
	quadRitem->Instances.resize(4);
	for (int i = 0; i < 4; i++)
	{
		XMStoreFloat4x4(&quadRitem->Instances[i].World, XMMatrixTranslation(i / 2.0f, 0.0f, 0.0f));
	}
	InstanceIndex += quadRitem->Instances.size();
	mRenderItemLayer[(int)RenderLayer::Debug].push_back(quadRitem.get());
	mAllRitems.push_back(std::move(quadRitem));


	auto skyCubeRitm = std::make_unique<RenderItem>();
	skyCubeRitm->ObjCBIndex = InstanceIndex++;
	skyCubeRitm->Geo = mGeometries["shapeGeo"].get();
	skyCubeRitm->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyCubeRitm->IndexCount = skyCubeRitm->Geo->DrawArgs["sphere"].IndexCount;
	skyCubeRitm->StartIndexLocation = skyCubeRitm->Geo->DrawArgs["sphere"].StartIndexLocation;
	skyCubeRitm->BaseVertexLocation = skyCubeRitm->Geo->DrawArgs["sphere"].BaseVertexLocation;
	skyCubeRitm->Instances.resize(1);
	XMStoreFloat4x4(&skyCubeRitm->Instances[0].World, XMMatrixScaling(500.0f, 500.0f, 500.0f)*XMMatrixTranslation(0.0f,0.0f,0.0f));
	skyCubeRitm->Instances[0].MaterialIndex = 0;
	mRenderItemLayer[(int)RenderLayer::SkyCube].push_back(skyCubeRitm.get());
	mAllRitems.push_back(std::move(skyCubeRitm));


	auto DepthRitem = std::make_unique<RenderItem>();
	DepthRitem->ObjCBIndex = InstanceIndex;
	DepthRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	DepthRitem->Geo = mGeometries["shapeGeo"].get();
	DepthRitem->IndexCount = DepthRitem->Geo->DrawArgs["quad"].IndexCount;
	DepthRitem->StartIndexLocation = DepthRitem->Geo->DrawArgs["quad"].StartIndexLocation;
	DepthRitem->BaseVertexLocation = DepthRitem->Geo->DrawArgs["quad"].BaseVertexLocation;
	DepthRitem->Instances.resize(1);
	InstanceIndex += DepthRitem->Instances.size();
	XMStoreFloat4x4(&DepthRitem->Instances[0].World, XMMatrixTranslation(0.0, 0.5f, 0.0f));
	mRenderItemLayer[(int)RenderLayer::Depth].push_back(DepthRitem.get());
	mAllRitems.push_back(std::move(DepthRitem));

	maxInstanceNum = InstanceIndex;
}

