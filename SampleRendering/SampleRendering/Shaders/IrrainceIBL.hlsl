#include"LightUtil.hlsl"

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
    float cbPerPassPad1;
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

float4 PS(VertexOut vout) :SV_Target
{
    float3 N = normalize(vout.PosL);

    float3 irradiance = float3(0.0f,0.0f,0.0f);
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = cross(up, N);
    up = cross(N, right);
    float sampleDelta = 0.025f;
    float nrSamples = 0.0f;
    for (float phi = 0.0; phi < 2.0 * PI;phi+=sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta +=sampleDelta)
        {
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
            
            irradiance += gCubeMap.Sample(gsamLinearWrap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    
    float4 FragColor = float4(irradiance, 1.0);
    return FragColor;
}