#ifndef NUM_DIR_LIGHTS
	#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
	#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
	#define NUM_SPOT_LIGHTS 0
#endif

#include "LightUtil.hlsl"


//Constant  data that varies per frame.
Texture2D gDiffuseMap[5] : register(t0,space0);

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);

TextureCube gCubeMap : register(t0,space4);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

//Constant data that varies per material.
cbuffer cbPass : register(b0)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float roughness;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;
    //雾相关参数
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
   
    Light gLights[MaxLights];
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
};
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : PSITION;
};
VertexOut VS(VertexIn vin,uint instanceID:SV_InstanceID)
{
    //取回实例化数据
    InstanceData instData = gInstanceData[instanceID];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    uint matIndex = instData.MaterialIndex;
    
    VertexOut vout;

    vout.PosL = vin.PosL;

    float4 posW = mul(float4(vin.PosL, 1.0f),world);

    posW.xyz += gEyePosW;
    
    vout.PosH = mul(posW, gViewProj).xyww;
 
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 color = gCubeMap.SampleLevel(gsamLinearWrap, pin.PosL,0);
    return color;
    
}