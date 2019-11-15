#include "GeometryGenerator.h"
#include <direct.h>
#include "DirectXMath.h"
#include <algorithm>
using namespace DirectX;
//sliceCount 切片数量，stackCount 切割的层数
GeometryGenerator::MeshData
GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius,
	float height, uint32 sliceCount, uint32 stackCount)
{
	
#define XM_PI 3.141592654f
		MeshData meshData;
		//构建堆叠层
		float stackHeight = height / stackCount;//每一层的高度
		//计算从下至上每一相邻分层使所需的半径增量
		float radiusStep = (topRadius - bottomRadius) / stackCount;
		uint32 ringCount = stackCount + 1;
		//从地面至上计算每个堆叠层环上的顶点坐标
		for (uint32 i = 0; i < ringCount; i++)
		{
			float y = -0.5f*height + i * stackHeight;
			float r = bottomRadius + i * radiusStep;
			//环上的各个顶点
			float dTheta = 2.0f*XM_PI / sliceCount;
			for (uint32 j=0;j<=sliceCount;j++)
			{
				Vertex vertex;
				float c = cosf(j*dTheta);
				float s = sinf(j*dTheta);

				vertex.Position = XMFLOAT3(r*c, y, r*s);

				vertex.TexC.x = (float)j / sliceCount;
				vertex.TexC.y = 1.0f - (float)i / stackCount;

				vertex.TangentU = XMFLOAT3(-s, 0.0f, c);

				float dr = bottomRadius - topRadius;
				XMFLOAT3 bitangent(dr*c, -height, dr*s);

				XMVECTOR T = XMLoadFloat3(&vertex.TangentU);
				XMVECTOR B = XMLoadFloat3(&bitangent);
				XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));

				XMStoreFloat3(&vertex.Normal, N);
				meshData.Vertices.push_back(vertex);
			}
		}
		//+1是希望让每环的第一个顶点和最后一个顶点重合，
		//这是因为它们的纹理坐标并不相同
		uint32 ringVertexCount = sliceCount+1;
		//计算每个侧面中三角形的索引
		for (uint32 i=0;i<stackCount;i++)
		{
			for (uint32 j=0;j<sliceCount;j++)
			{
				meshData.Indices32.push_back(i*ringVertexCount + j);
				meshData.Indices32.push_back((i + 1)*ringVertexCount + j);
				meshData.Indices32.push_back((i + 1)*ringVertexCount + j + 1);

				meshData.Indices32.push_back(i*ringVertexCount + j);
				meshData.Indices32.push_back((i + 1)*ringCount + j + 1);
				meshData.Indices32.push_back(i*ringCount + j + 1);
			}
		}
		BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount,meshData);
		BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount,meshData);
	
	return meshData;
}
void GeometryGenerator::BuildCylinderBottomCap(float bottomRadius, float topRadius, float height,
	uint32 sliceCount, uint32 stackCount, MeshData& meshData)
{
	uint32 baseIndex = (uint32)meshData.Vertices.size();
	
	float y = 0.5f*height;
	float dTheta = 2.0f*XM_PI / sliceCount;
	//使圆台端面环上的首尾顶点重合，因为纹理坐标与法线并不同
	for (int i=0;i<=sliceCount;i++)
	{
		float x = topRadius * cosf(i*dTheta);
		float z = topRadius * sinf(i*dTheta);
		//根据圆台的高度使用顶面纹理坐标范围按比例缩小
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}
	//顶面的中心坐标
	meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	//中心顶点的索引值

	uint32 centerIndex = (uint32)meshData.Vertices.size() - 1;

	for (uint32 i=0;i<sliceCount;i++)
	{
		meshData.Indices32.push_back(centerIndex);
		meshData.Indices32.push_back(baseIndex + i + 1);
		meshData.Indices32.push_back(baseIndex + i);
	}
}
void GeometryGenerator::BuildCylinderTopCap(float bottomRadius, float topRadius, float height,
	uint32 sliceCount, uint32 stackCount, MeshData& meshData)
{
	uint32 baseIndex = (uint32)meshData.Vertices.size();

	float y = -0.5f*height;
	float dTheta = 2.0f*XM_PI / sliceCount;
	//使圆台端面环上的首尾顶点重合，因为纹理坐标与法线并不同
	for (int i = 0; i <= sliceCount; i++)
	{
		float x = bottomRadius * cosf(i*dTheta);
		float z = bottomRadius * sinf(i*dTheta);
		//根据圆台的高度使用顶面纹理坐标范围按比例缩小
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}
	//顶面的中心坐标
	meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	//中心顶点的索引值

	uint32 centerIndex = (uint32)meshData.Vertices.size() - 1;

	for (uint32 i = 0; i < sliceCount; i++)
	{
		meshData.Indices32.push_back(centerIndex);
		meshData.Indices32.push_back(baseIndex + i);
		meshData.Indices32.push_back(baseIndex + i+1);
	}
}


GeometryGenerator::MeshData GeometryGenerator::CreateGeosphere(float radius, uint32 numSubdivisions)
{
	MeshData meshData;
	//确定细分的次数
	numSubdivisions = std::min<uint32>(numSubdivisions, 6u);
	//通过一个正二十面体进行曲面细分来逼近一个球体
	//利用二十面体的顶点计算公式计算各个顶点
	//若正二十面体中心为（0，0，0），各个顶点坐标为{(±X,0,±Z), (0,±Z,±X), (±Z,±X,0)}
	//X= ((50-10*(5**0.5))**0.5)/10   Z=((50+10*(5**0.5))**0.5)/10
	const float X = 0.525731f;
	const float Z = 0.850651f;
	XMFLOAT3 pos[12] =
	{
		XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),
		XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
		XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X),
		XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),
		XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f),
		XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
	};
	//二十个面一共使用了12个点搭建，索引为20个面（60个索引）
	uint32 k[60] =
	{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};
	meshData.Vertices.resize(12);
	meshData.Indices32.assign(&k[0], &k[60]);
	for (uint32 i=0;i<12;i++)
	{
		meshData.Vertices[i].Position = pos[i];
	}
	for (uint32 i=0;i<numSubdivisions;i++)
	{
		Subdivide(meshData);
	}
	//将每一个顶点投影到球面，并推导出对应的纹理坐标
	for (uint32 i=0;i<meshData.Vertices.size();i++)
	{
		//投影到单位球
		XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&meshData.Vertices[i].Position));
		//投影到球上
		XMVECTOR p = radius * n;
		XMStoreFloat3(&meshData.Vertices[i].Position, p);
		XMStoreFloat3(&meshData.Vertices[i].Normal, n);
		//由球面坐标生成纹理坐标
		float theta = atan2f(meshData.Vertices[i].Position.z, meshData.Vertices[i].Position.x);
		//将theta限制到0~2pi
		if (theta<0.0f)
		{
			theta += XM_2PI;
		}
		float phi = acosf(meshData.Vertices[i].Position.y / radius);

		meshData.Vertices[i].TexC.x = theta / XM_2PI;
		meshData.Vertices[i].TexC.y = phi / XM_2PI;
	
		//求出P关于theta的偏导数
		meshData.Vertices[i].TangentU.x = -radius * sinf(phi)*sinf(theta);
		meshData.Vertices[i].TangentU.y = 0.0f;
		meshData.Vertices[i].TangentU.z = +radius * sinf(phi)*cosf(theta);
		
		XMVECTOR T = XMLoadFloat3(&meshData.Vertices[i].TangentU);
		XMStoreFloat3(&meshData.Vertices[i].TangentU, XMVector3Normalize(T));
	}
	return meshData;
}
void GeometryGenerator::Subdivide(MeshData& meshData)
{
	//保存输入几何体的备份
	MeshData inputCopy = meshData;

	meshData.Vertices.resize(0);
	meshData.Indices32.reserve(0);
	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2     v2

	uint32 numTris = (uint32)inputCopy.Indices32.size()/3;//三角形数量
	for (uint32 i=0;i<numTris;i++)
	{
		//每隔三个点取三角形
		Vertex v0 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 0]];
		Vertex v1 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 1]];
		Vertex v2 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 2]];
		//
		//生成中点
		//
		Vertex m0 = MidPoint(v0, v1);
		Vertex m1 = MidPoint(v1, v2);
		Vertex m2 = MidPoint(v0, v2);
		//
		//增加新的几何体
		//
		meshData.Vertices.push_back(v0); // 0
		meshData.Vertices.push_back(v1); // 1
		meshData.Vertices.push_back(v2); // 2
		meshData.Vertices.push_back(m0); // 3
		meshData.Vertices.push_back(m1); // 4
		meshData.Vertices.push_back(m2); // 5

		meshData.Indices32.push_back(i * 6 + 0);
		meshData.Indices32.push_back(i * 6 + 3);
		meshData.Indices32.push_back(i * 6 + 5);

		meshData.Indices32.push_back(i * 6 + 3);
		meshData.Indices32.push_back(i * 6 + 4);
		meshData.Indices32.push_back(i * 6 + 5);

		meshData.Indices32.push_back(i * 6 + 5);
		meshData.Indices32.push_back(i * 6 + 4);
		meshData.Indices32.push_back(i * 6 + 2);

		meshData.Indices32.push_back(i * 6 + 3);
		meshData.Indices32.push_back(i * 6 + 1);
		meshData.Indices32.push_back(i * 6 + 4);
	}
}
GeometryGenerator::Vertex GeometryGenerator::MidPoint(const Vertex& v0, const Vertex& v1)
{
	XMVECTOR p0 = XMLoadFloat3(&v0.Position);
	XMVECTOR p1 = XMLoadFloat3(&v1.Position);

	XMVECTOR n0 = XMLoadFloat3(&v0.Normal);
	XMVECTOR n1 = XMLoadFloat3(&v1.Normal);

	XMVECTOR tan0 = XMLoadFloat3(&v0.TangentU);
	XMVECTOR tan1 = XMLoadFloat3(&v1.TangentU);

	XMVECTOR tex0 = XMLoadFloat2(&v0.TexC);
	XMVECTOR tex1 = XMLoadFloat2(&v1.TexC);
	//为中点计算所有属性，并且Vecotrs需要被归一化normalized
	XMVECTOR pos = 0.5f*(p0 + p1);
	XMVECTOR normal = XMVector3Normalize(0.5f*(n0 + n1));
	XMVECTOR tangent = XMVector3Normalize(0.5f*(tan0 + tan1));
	XMVECTOR tex = 0.5f *(tex0 + tex1);

	Vertex v;
	XMStoreFloat3(&v.Position, pos);
	XMStoreFloat3(&v.Normal, normal);
	XMStoreFloat3(&v.TangentU, tangent);
	XMStoreFloat2(&v.TexC, tex);

	return v;
}
GeometryGenerator::MeshData
GeometryGenerator::CreateSphere(float radius, uint32 sliceCount, uint32 stackCount)
{
	MeshData meshData;
	//计算顶部的顶点，并且向下移动栈

	//两极：注意到纹理坐标会失真，因为当矩形纹理投射到球面时可指定到两极点的坐标不唯一。
	Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	meshData.Vertices.push_back(topVertex);

	float phiStep = XM_PI / stackCount;
	float thetaStep = 2.0f*XM_PI / sliceCount;

	//计算每一个栈环上的顶点（极点不即为环）
	for (uint32 i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i * phiStep;
		// Vertices of ring.
		for (uint32 j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			Vertex v;
			//由球形到笛卡尔
			v.Position.x = radius * sinf(phi)*cosf(theta);
			v.Position.y = radius * cosf(phi);
			v.Position.z = radius * sinf(phi)*sinf(theta);

			//P相对于 theta的偏导数
			v.TangentU.x = -radius * sinf(phi)*sinf(theta);
			v.TangentU.y = 0.0f;
			v.TangentU.z = +radius * sinf(phi)*cosf(theta);

			XMVECTOR T = XMLoadFloat3(&v.TangentU);
			XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

			XMVECTOR p = XMLoadFloat3(&v.Position);
			XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

			v.TexC.x = theta / XM_2PI;
			v.TexC.y = phi / XM_PI;

			meshData.Vertices.push_back(v);
		}
	}
	meshData.Vertices.push_back(bottomVertex);

	//计算顶层堆栈的索引。
	//顶部堆栈首先写入顶点缓冲区并将顶部极点连接到第一个环。
	for (uint32 i = 1; i <= sliceCount; ++i)
	{
		meshData.Indices32.push_back(0);
		meshData.Indices32.push_back(i + 1);
		meshData.Indices32.push_back(i);
	}
	//计算内部栈的索引（不与极点相连）
	
	//将索引偏移到第一个环中第一个顶点的索引。

	uint32 baseIndex = 1;
	uint32 ringVertexCount = sliceCount + 1;
	for (uint32 i = 0; i < stackCount - 2; ++i)
	{
		for (uint32 j = 0; j < sliceCount; ++j)
		{
			meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j);
			meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
			meshData.Indices32.push_back(baseIndex + (i + 1)*ringVertexCount + j);

			meshData.Indices32.push_back(baseIndex + (i + 1)*ringVertexCount + j);
			meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
			meshData.Indices32.push_back(baseIndex + (i + 1)*ringVertexCount + j + 1);
		}
	}
	//为顶部栈计算索引。顶部的栈是最后被写入顶点缓冲的
	//链接顶部的极与环

	//北极点最后被添加
	uint32 southPoleIndex = (uint32)meshData.Vertices.size() - 1;
	//偏移索引到最后一个环上的第一个顶点
	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		meshData.Indices32.push_back(southPoleIndex);
		meshData.Indices32.push_back(baseIndex + i);
		meshData.Indices32.push_back(baseIndex + i + 1);
	}

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateBox(float width, float height, float depth, uint32 numSubdivisions)
{
	MeshData meshData;

	//
	// Create the vertices.
	//

	Vertex v[24];

	float w2 = 0.5f*width;
	float h2 = 0.5f*height;
	float d2 = 0.5f*depth;

	// Fill in the front face vertex data.
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the back face vertex data.
	v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the top face vertex data.
	v[8] = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9] = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face vertex data.
	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face vertex data.
	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the right face vertex data.
	v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	meshData.Vertices.assign(&v[0], &v[24]);

	//
	// Create the indices.
	//

	uint32 i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	meshData.Indices32.assign(&i[0], &i[36]);

	// Put a cap on the number of subdivisions.
	numSubdivisions = std::min<uint32>(numSubdivisions, 6u);

	for (uint32 i = 0; i < numSubdivisions; ++i)
		Subdivide(meshData);

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGrid(float width, float depth, uint32 m, uint32 n)
{
	MeshData meshData;
	uint32 vertexCount = m * n;
	uint32 faceCount = (m - 1)*(n - 1) * 2;
	//
	float halfWidth = 0.5f*width;
	float halfDepth = 0.5f*depth;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);
	meshData.Vertices.resize(vertexCount);
	for (uint32 i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dz;
		for (uint32 j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;

			meshData.Vertices[i*n + j].Position = XMFLOAT3(x, 0.0f, z);
			meshData.Vertices[i*n + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			meshData.Vertices[i*n + j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

			// 在网格上伸缩纹理
			meshData.Vertices[i*n + j].TexC.x = j * du;
			meshData.Vertices[i*n + j].TexC.y = i * dv;
		}
	}
	//创建索引
	meshData.Indices32.resize(faceCount * 3); // 3 indices per face

// 遍历每个四边形并计算索引
	uint32 k = 0;
	for (uint32 i = 0; i < m - 1; ++i)
	{
		for (uint32 j = 0; j < n - 1; ++j)
		{
			meshData.Indices32[k] = i * n + j;
			meshData.Indices32[k + 1] = i * n + j + 1;
			meshData.Indices32[k + 2] = (i + 1)*n + j;

			meshData.Indices32[k + 3] = (i + 1)*n + j;
			meshData.Indices32[k + 4] = i * n + j + 1;
			meshData.Indices32[k + 5] = (i + 1)*n + j + 1;

			k += 6; // 下一个四边形
		}
	}

	return meshData;
}
GeometryGenerator::MeshData GeometryGenerator::CreateQuad(float x, float y, float w, float h, float depth)
{
	MeshData meshData;

	meshData.Vertices.resize(4);
	meshData.Indices32.resize(6);

	// Position coordinates specified in NDC space.
	meshData.Vertices[0] = Vertex(
		x, y - h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f);

	meshData.Vertices[1] = Vertex(
		x, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f);

	meshData.Vertices[2] = Vertex(
		x + w, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f);

	meshData.Vertices[3] = Vertex(
		x + w, y - h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f);

	meshData.Indices32[0] = 0;
	meshData.Indices32[1] = 1;
	meshData.Indices32[2] = 2;

	meshData.Indices32[3] = 0;
	meshData.Indices32[4] = 2;
	meshData.Indices32[5] = 3;

	return meshData;
}