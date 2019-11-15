#include "LightUtil.hlsl"
Texture2D gTextureMaps[25] : register(t0,space0);

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);

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
    //����ز���
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
    //ʹ��nointerpolation���η�����˸�����ָ��Ķ���δ����ֵ�������Ρ�
    nointerpolation uint MatIndex : MATINDEX;
};

struct PSOutput
{
    float4 Normal : SV_Target;
    float4 PosS : SV_Target1;
    float4 BaseColorAlbedo : SV_Target2;
    float4 RoughnessMetallicAO : SV_Target3;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;
    
 
  
    //ȡ��ʵ������
    InstanceData instData = gInstanceData[instanceID];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    uint matIndex = instData.MaterialIndex;

    vout.MatIndex = matIndex;
    vout.TangentW = mul(vin.TangentL, (float3x3) world);


    MaterialData materialData = gMaterialData[matIndex];
	//�л�������ռ�
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosW = posW.xyz;
	//��������˷Ǿ������ţ����仰˵�����Ǿ���Ҫʹ�� �������� inverse-transpose 
    vout.NormalW = mul(vin.NormalL, (float3x3) world);

	//�л�����βü��ռ�
    vout.PosH = mul(posW, gViewProj);
    //
    float4 texC = mul(float4(vin.Tec, 0.0f, 1.0f), texTransform);
    vout.Tec = mul(texC, materialData.MatTransform).xy;
    vout.Tec = texC.xy;
    return vout;
}

PSOutput PS(VertexOut pin) : SV_Target1
{
    PSOutput PSout;
        //ȡ�ز�������
    MaterialData matData = gMaterialData[pin.MatIndex];
    uint AlbeoMapIndex = matData.AlbeoMapIndex;
    uint NormalMapIndex = matData.NormalMapIndex;
    uint MetalMapIndex = matData.MetalMapIndex;
    uint RoughnessMapIndex = matData.RoughnessMapIndex;
    uint AOMapIndex = matData.AOMapIndex;
    float3 Fresnel = matData.FresnelR0;
	//�����˲�ֵ��ķ�������������Ҫ�ٴ� �淶��
    pin.NormalW = normalize(pin.NormalW);

    float4 Albeo = gTextureMaps[AlbeoMapIndex].Sample(gsamAnisotropicWrap, pin.Tec);
    float MetalMapSample = gTextureMaps[MetalMapIndex].Sample(gsamAnisotropicWrap, pin.Tec);
    float4 RoughnessMapSample = gTextureMaps[RoughnessMapIndex].Sample(gsamAnisotropicWrap, pin.Tec);
    float4 AOMapSample = gTextureMaps[AOMapIndex].Sample(gsamAnisotropicWrap, pin.Tec);
    //ȡ�ط������ǻ���Ҫ�����������߿ռ�ת����������ϵ��
    float4 normalMapSample = gTextureMaps[NormalMapIndex].Sample(gsamAnisotropicWrap, pin.Tec);
    float3 normal = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
   
    PSout.Normal = normalize(float4(normal, 1.0));
    PSout.PosS = float4(pin.PosW, 1.0f);
    PSout.BaseColorAlbedo = Albeo;
    PSout.RoughnessMetallicAO = float4(RoughnessMapSample.x, MetalMapSample, AOMapSample.x, 1.0f);

    return PSout;
}
