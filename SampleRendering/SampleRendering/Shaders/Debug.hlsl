#include"LightUtil.hlsl"

Texture2D gDebugMap[4] : register(t0,space0);

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);



SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);


struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tec : TEXCOORD;
    float3 TangentL : TANGENT;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 Tec : TEXCOORD;

    nointerpolation uint MatIndex : MATINDEX;
};

VertexOut VS(VertexIn vin,uint instanceID : SV_InstanceID)
{
      
    //取回实例数据
    InstanceData instData = gInstanceData[instanceID];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    uint matIndex = instData.MaterialIndex;

	VertexOut vout = (VertexOut)0.0f;

    // Already in homogeneous clip space.
    vout.PosH = mul(float4(vin.PosL, 1.0f), world);
	
    vout.Tec = vin.Tec;
    vout.MatIndex = instanceID;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(gDebugMap[pin.MatIndex].Sample(gsamLinearWrap, pin.Tec).rgb, 1.0f);
}


