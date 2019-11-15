#ifndef NUM_DIR_LIGHTS
	#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
	#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
	#define NUM_SPOT_LIGHTS 0
#endif

#include "LightUtil.hlsl"

#define ROUGHNESSLEVELS 6
//Constant  data that varies per frame.
Texture2D gTextureMaps[26] : register(t0);

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);


Texture2D gDisplacementMap : register(t0,space2);

TextureCube gCubeMap : register(t0, space4);
TextureCube gDiffuseMap : register(t0, space5);
TextureCube gSpecularMap : register(t0, space6);

Texture2D gShadowMaps[4] : register(t0,space7);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

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
    //使用nointerpolation修饰符，因此该索引指向的都是未经插值的三角形。
    nointerpolation uint MatIndex : MATINDEX;
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
VertexOut VS(VertexIn vin,uint instanceID:SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;
    
 
  
    //取回实例数据
    InstanceData instData = gInstanceData[instanceID];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    uint matIndex = instData.MaterialIndex;

    vout.MatIndex = matIndex;
    vout.TangentW = mul(vin.TangentL, (float3x3)world);


    MaterialData materialData = gMaterialData[matIndex];
	//切换到世界空间
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosW = posW.xyz;
	//假设进行了非均匀缩放，换句话说，我们就需要使用 世界矩阵的 inverse-transpose 
    vout.NormalW = mul(vin.NormalL, (float3x3) world);

	//切换到齐次裁剪空间
    vout.PosH = mul(posW, gViewProj);
    //
    float4 texC = mul(float4(vin.Tec, 0.0f, 1.0f), texTransform);
    vout.Tec = mul(texC, materialData.MatTransform).xy;
    vout.Tec = texC.xy;
    return vout;
}
float4 PS(VertexOut pin) : SV_Target
{

    //取回材质数据
    MaterialData matData = gMaterialData[pin.MatIndex];
    uint AlbeoMapIndex = matData.AlbeoMapIndex;
    uint NormalMapIndex = matData.NormalMapIndex;
    uint MetalMapIndex = matData.MetalMapIndex;
    uint RoughnessMapIndex = matData.RoughnessMapIndex;
    uint AOMapIndex = matData.AOMapIndex;
    float3 Fresnel = matData.FresnelR0;
	//进行了插值后的法线量后，所以需要再次 规范化
    pin.NormalW = normalize(pin.NormalW);
   
	//从贴图样本中取回数据
    float4 test = gDisplacementMap.Sample(gsamAnisotropicWrap, pin.Tec);
    float4 Albeo = gTextureMaps[AlbeoMapIndex].Sample(gsamAnisotropicWrap, pin.Tec);
    float MetalMapSample = gTextureMaps[MetalMapIndex].Sample(gsamAnisotropicWrap, pin.Tec);
    float4 RoughnessMapSample = gTextureMaps[RoughnessMapIndex].Sample(gsamAnisotropicWrap, pin.Tec);
    float4 AOMapSample = gTextureMaps[AOMapIndex].Sample(gsamAnisotropicWrap, pin.Tec);
    //取回法线我们还需要讲法线由切线空间转到世界坐标系下
    float4 normalMapSample = gTextureMaps[NormalMapIndex].Sample(gsamAnisotropicWrap, pin.Tec);
    float3 normal = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);

    float3 toEyeW = gEyePosW - pin.PosW;

    float2 LUTtex = float2(max(dot(normal, toEyeW), 0.0), RoughnessMapSample.x);
    float3 LUT = gTextureMaps[25].Sample(gsamLinearClamp, LUTtex).rgb;


    float distToEye = length(toEyeW);
    toEyeW /= distToEye;

    float3 directLight = float3(0.0f,0.0f,0.0f);

    float Metal = MetalMapSample;
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(float3(1.0, 1.0, 1.0), Albeo.rgb, Metal);
    F0 = Albeo.rgb;
    float3 r = reflect(-toEyeW, pin.NormalW);

    float3 diffuseEnv = gDiffuseMap.Sample(gsamLinearWrap, normal);
    float3 prefilteredColor = gSpecularMap.SampleLevel(gsamLinearWrap, r, RoughnessMapIndex.x * ROUGHNESSLEVELS);
    
    for (int i = 0; i < 1;i++)
    {
        float shadowFactor = CalcShadowFactor(float4(pin.PosW, 1.0), i);

        float3 lightVec = normalize(gLights[i].Position - pin.PosW);
        float distance = length(gLights[i].Position - pin.PosW);
        float attenuation = 1.0 /( distance );
        directLight = directLight + DirectLightSG(Metal, Albeo.xyz, F0, normal, lightVec, RoughnessMapSample.x, toEyeW, shadowFactor);
   
    }
     
    float3 ks = F_SchlikRoughness(F0, RoughnessMapSample.x, dot(normal, toEyeW));
    float3 kd = 1.0 - ks;
    kd *= 1.0 - Metal;


    #ifdef ALPHA_TEST
        clip(diffuseAlbeo.a-0.1f);
    #endif

    float3 envLight = prefilteredColor * (ks * LUT.x + LUT.y) + kd * diffuseEnv*Albeo.rgb;

    float3 litColor = envLight * AOMapSample.r+directLight;
    #ifdef FOG
    float fogAmount =saturate((distToEye-gFogStart)/gFogRange);
  //  litColor =lerp(litColor,gFogColor,fogAmount);
    #endif

  //  litColor = litColor / (litColor + float3(1.0, 1.0, 1.0));
    //伽马校正
    litColor = pow(litColor, 1 / 1.5);
    normal = normalize(normal);
    return float4(envLight, 1.0f);
};