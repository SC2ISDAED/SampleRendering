#define ROUGHNESSLEVELS 6
#include"LightUtil.hlsl"
#define MaxLights 4
Texture2D gTextureMaps[5] : register(t0);
TextureCube gDiffuseMap : register(t0, space1);
TextureCube gSpecularMap : register(t0, space2);
Texture2D LUTMap : register(t0, space3);

Texture2D gShadowMaps[4] : register(t0, space4);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

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

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

float CalcShadowFactor(float4 shadowPosH, int index)
{
    shadowPosH = mul(shadowPosH, gLights[index].shadowTransform);
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;

    uint width, height, numMips;
    gShadowMaps[index].GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float) width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += gShadowMaps[index].SampleCmpLevelZero(gsamShadow,
            shadowPosH.xy + offsets[i], depth).r;
    }
    

    return percentLit / 9.0f;
}


struct VertexOut
{

    float4 PosL : SV_POSITION;
    float2 TexC : TEXCOORD;
};
VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
    vout.TexC = gTexCoords[vid];

    vout.PosL = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.01, 1.0f);

    return vout;
}
float4 PS(VertexOut pin) : SV_Target
{
    //Gbuffer中的数据
    
    float3 normal = gTextureMaps[0].Sample(gsamAnisotropicWrap, pin.TexC).rgb;
    normal = normalize(normal);
    float3 position = gTextureMaps[1].Sample(gsamAnisotropicWrap, pin.TexC).rgb;
    float4 Albeo = gTextureMaps[2].Sample(gsamAnisotropicWrap, pin.TexC);
    float3 RoughnessMetallicAO = gTextureMaps[3].Sample(gsamAnisotropicWrap, pin.TexC).rgb;

    float Roughness = RoughnessMetallicAO.r;
    float Metallic = RoughnessMetallicAO.g;
    float AO = RoughnessMetallicAO.b;

    float3 toEyeW = gEyePosW - position;

    float2 LUTtex = float2(max(dot(normal, toEyeW), 0.0), Roughness);
    float3 LUT = LUTMap.Sample(gsamLinearClamp, LUTtex).rgb;


    float distToEye = length(toEyeW);
    toEyeW /= distToEye;

    float3 directLight = float3(0.0f, 0.0f, 0.0f);

    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(float3(1.0, 1.0, 1.0), Albeo.rgb, Metallic);
    F0 = Albeo.rgb;
    float3 r = reflect(-toEyeW, normal);

    float3 diffuseEnv = gDiffuseMap.Sample(gsamLinearWrap, normal);
    float3 prefilteredColor = gSpecularMap.SampleLevel(gsamLinearWrap, r, Roughness * ROUGHNESSLEVELS);
    float shadowFactor = 0.0f;
    shadowFactor = CalcShadowFactor(float4(position, 1.0), 0);
    for (int i = 0; i < 1; i++)
    {

        float3 lightVec = normalize(gLights[i].Position -position);
        float distance = length(gLights[i].Position - position);
        float attenuation = 1.0 / (distance);
        directLight = directLight + DirectLightSG(Metallic, Albeo.xyz, F0, normal, lightVec, Roughness, toEyeW, shadowFactor);
      //  directLight += DirectLight(Metallic, Albeo.xyz, F0, normal, lightVec, Roughness, toEyeW);

    }

    float3 ks = F_SchlikRoughness(F0,roughness, dot(normal, toEyeW));
    float3 kd = 1.0 - ks;
    kd *= 1.0 - Metallic;


#ifdef ALPHA_TEST
        clip(diffuseAlbeo.a-0.1f);
#endif

    float3 envLight = prefilteredColor * (ks * LUT.x + LUT.y)  + kd * diffuseEnv * Albeo.rgb;

    float3 litColor = envLight * AO + directLight;
#ifdef FOG
    float fogAmount =saturate((distToEye-gFogStart)/gFogRange);
  //  litColor =lerp(litColor,gFogColor,fogAmount);
#endif

  //  litColor = litColor / (litColor + float3(1.0, 1.0, 1.0));
    //伽马校正
    litColor = pow(litColor, 1 / 1.5);
    return float4(litColor, 1.0);
};