#define MaxLights 4
#include<BRDF.hlsl>
#include<SGLightSource.hlsl>
struct Light
{
	float3 Strength;
	float FalloffStart;
	float3 Direction;
	float FalloffEnd;
	float3 Position;
	float SpotPower;
    float4x4 shadowTransform;
};
struct Material
{
	float4 DiffuseAlbeo;
	float3 FresenelR0;
	float Shininess;
};
struct InstanceData
{
    float4x4 World;
    float4x4 TexTransform;
	float4x4 LastWorld;
    uint MaterialIndex;
    uint InstPad0;
    uint InstPad1;
    uint InstPad2;
};
struct MaterialData
{
    float3 FresnelR0;
    float pad ;

    uint AlbeoMapIndex;
    uint NormalMapIndex;
    uint MetalMapIndex;
    uint RoughnessMapIndex;
    uint AOMapIndex;

    float4x4 MatTransform;
};
float CalcAttenuation(float d, float fallofStart, float fallofEnd)
{
	//线性衰减
	//saturate 函数，将值截断在 0~1 ；
	return saturate((fallofEnd - d) / (fallofEnd - fallofStart));
}
float MoreControlAttenuation(float d,float constValue,float linearValue,float squareValue)
{
	float downer = constValue + linearValue * d + squareValue * d*d;
	return saturate(1 / downer);
}

// BlinnPhong
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize((lightVec + toEye));

    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 frensnelFactor = F_Schlick(mat.FresenelR0, halfVec, lightVec);

    float3 specAlbedo = frensnelFactor * roughnessFactor;

    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbeo.rgb + specAlbedo) * lightStrength;
}
// 对方向光计算等式
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
	// 光照射方向，是光位置向量的相反向量
    float3 lightVec = -L.Direction;
	//根据 Lambert‘s cosine low 进行缩放
    float ndotl = max(dot(normal, lightVec), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);

}
//为点光源计算等式
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
	//计算表面到光源的向量
    float3 lightVec = L.Position - pos;

	//计算表面到光的距离
    float d = length(lightVec);

	//范围测试
    if (d>L.FalloffEnd)
        return 0.0f;
	//Normalize light vector
    lightVec /= d;
		//根据 Lambert‘s cosine low 进行缩放
    float ndotl = max(dot(normal, lightVec), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

	//光源衰减
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);

}

float4 ComputeLighting(Light gLights[MaxLights], Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    float3 result = 0.0f;

    int i = 0;

#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif

#if (NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS+NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif 

    return float4(result, 0.0f);
}

float3 NormalSampleToWorldSpace(float3 normalMapSample,float3 uintNormalW,float3 tangentW)
{
    //解压至-1~1
    float3 normal = 2.0f * normalMapSample - 1.0f;
    //规范正交化坐标基
    float3 N = uintNormalW;
    float3 T = normalize(tangentW - dot(N, tangentW) * N);
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);

    float3 bumpedNormalW = mul(normal, TBN);
    return normalize(bumpedNormalW);
}

float3 DirectLight(float metallic,float3 diffuseAlbeo,float3 F0,float3 Normal,
                    float3 lightVec,float roughness,float3 View)
{
    //计算半向量
    float3 HalfVector = normalize(lightVec + View);
    float a2 = roughness * roughness;

    float NdotH = dot(Normal, HalfVector);
    float NdotL = dot(Normal, lightVec);
    float NdotthusL = dot(Normal, -lightVec);
    float NdotV = dot(Normal, View);
    float VdotH = dot(HalfVector, View);

    float3 F = F_Schlick(F0, HalfVector,View);

    float G = G_Schlick_GGX_G2(roughness, lightVec, Normal, View);
    
    float D = D_GGX(a2,NdotH);
    //计算表面反射
    float denominator = 4 * max(abs(NdotV) * abs(NdotL),0.01f);
    float3 Spcular = G * F * D / max(denominator, 0.001f);

    //计算次表面散射

    SG light = CosineLobeSG(lightVec);

    float3 diffuse = LambertDiffuse(F,diffuseAlbeo,metallic);
    float3 diffuseSG = SGIrradianceFitted(light, Normal);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD = kD * (1.0 - metallic);
    return Spcular * kS + diffuse ;
}
float3 DirectLightSG(float metallic, float3 diffuseAlbeo, float3 F0, float3 Normal,
                    float3 lightVec, float roughness, float3 View,float shadowFactor)
{
    //根据光源属性构造的SG结构体
    float3 HalfVector = normalize(lightVec + View);
    float3 F = F_Schlick(F0, HalfVector, View);

    SG light ;
    light.Axis = lightVec;
    light.Amplitude = 4.0f;
    light.Sharpness = 10.0f;
    float3 diffuseSG = SGIrradianceFitted(light, Normal);
    float3 Spcular = SpecularTermASGWarp(light, Normal, roughness, View, diffuseAlbeo);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD = kD * (1.0 - metallic);

    return Spcular * kS * shadowFactor + diffuseSG * kD * diffuseAlbeo*shadowFactor;

}