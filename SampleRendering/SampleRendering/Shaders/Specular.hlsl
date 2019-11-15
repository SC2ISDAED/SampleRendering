#include"LightUtil.hlsl"
#include"ImprotanceSample.hlsl"
StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);

TextureCube gCubeMap : register(t0, space4);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

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
};
struct VertexOut
{
    float3 PosL : POSITION;
    float4 PosH : SV_POSITION;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    InstanceData instData = gInstanceData[instanceID];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    uint matIndex = instData.MaterialIndex;


    VertexOut vout;
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosH = mul(posW, gViewProj).xyww;
    vout.PosL = vin.PosL;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    const uint NumSamples = 1024u;
    float3 N = normalize(pin.PosL);
    float3 R = N;
    float3 V = R;

    float3 prefilteredColor = float3(0.f, 0.f, 0.f);
    float totalWeight = 0.f;
    for (uint i = 0u; i < NumSamples; ++i)
    {
        float2 Xi = Hammersley(i, NumSamples);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0f * dot(V, H) * H - V);
        float NdotL = max(dot(N, L), 0.0f);
        if (NdotL > 0.f)
        {
            prefilteredColor += gCubeMap.Sample(gsamLinearWrap, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
   
    prefilteredColor = prefilteredColor / totalWeight;
    return float4(prefilteredColor, 1.0f);
}