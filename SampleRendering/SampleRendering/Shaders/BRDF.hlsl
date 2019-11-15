
#define PI 3.1415926

#define E  2.7182818
//-----------------------------------------PBR相关-------------------------------------------------

//              --------------------为表面反射建模------------------------------

//--------------------Fresnel 反射现象相关
//Fresnel 系数的计算采用的是SchlickFresnel 的方法
float3 F_Schlick(float3 F0, float3 HalfVecor, float3 View)
{
    float NdotL = saturate(dot(HalfVecor, View));
    float NdotL5 = pow((1.0f - NdotL), 5);
    return F0 + (1 - F0) * (NdotL5);
}
float3 F_SchlikRoughness(float3 F0,float roughness,float NdotV)
{
    float oRoughness = 1.0 - roughness;
    NdotV = max(NdotV, 0.0f);
    return F0 + (max(float3 (oRoughness, oRoughness, oRoughness), F0)-F0) * pow(1.0 - NdotV, 5.0);

}
//---------------------Specular NDF 法线分布相关实现
//1.BlinnPhong[blinn,1977]
//M  microfacet normal 
//N Macrofacet Normal 
float D_Blinn(float a2, float NoH)
{
    float n = 2 / a2 - 2;
    return (1 / (PI * a2)) * pow(NoH, n);
}
//2.Beckman分布
float D_Beckman(float a2, float NoH)
{
    float NoH2 = NoH * NoH;
    return pow(E, (NoH2 - 1) / (a2 * NoH2)) * (1 / (PI * a2 * NoH2 * NoH2));

}
//3.GGX(Trowbridge-Reitz)
float D_GGX(float a, float NoH)
{
    float a2 = a * a;
    NoH = max(NoH, 0.0f);
    float d = (NoH * a2 - NoH) * NoH + 1; // 2 mad
    return a2 / (PI * d * d);
}
//GGX NDF 各向异性版本
float D_GGXAnisotropic(float ax, float ay, float NoH, float3 H, float3 X, float3 Y)
{
    float XoH = dot(X, H);
    float YoH = dot(Y, H);
    float d = XoH * XoH / (ax * ax) + YoH * YoH / (ay * ay) + NoH * NoH;
    return 1 / (PI * ax * ay * d * d);
}

//4. GTR y=1  and y=2
float D_GTR1(float a2, float NoH)
{
    float kay = (a2 - 1) / log(a2);
    float dm = 1 / (PI * (1.0 + (a2 - 1.0) * (NoH * NoH)));
    return kay * dm;
}
float D_GTR2(float a2, float NoH)
{
    float kay = (a2 - 1) / (1 - 1 / a2);
    float dm = 1 / (PI * pow((1.0 + NoH * NoH * (a2 - 1)), 2));

}
//5.Blin-Phong NDF,这是以前广泛使用的方式，计算花费较低,a值越低越粗糙，范围是0~infi，并不利于artist 调动
float D_Blin_Phong(float a, float NoH)
{
    return max(NoH, 0) * (a + 2) * pow(NoH, a) / (2 * PI);
}
//--------------Specular G 相关实现
float G_Schlick_GGX_G1(float roughness,float3 NdotV)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV *( 1.0-k)+k;

    return nom / denom;

}

float G_Schlick_GGX_G2(float roughness, float3 lightVec, float3 normal,float3 ViewVec)
{
    float NdotL = max(dot(lightVec, normal), 0.0f);
    float NdotV = max(dot(ViewVec, normal), 0.0f);
    float ggx1 = G_Schlick_GGX_G1(roughness, NdotL);
    float ggx2 = G_Schlick_GGX_G1(roughness, NdotV);
    return ggx1*ggx2;
}


// -------------------------为次表面散射建模----------------------------
//光滑表面的次表面模型（表面下的散射距离小于表面的不规则度）
//1. Lambertian Model ，并没有考虑表面反射光不能用于散射
float3 LambertianModel(float3 albeo)
{
    return albeo / PI;
}
float3 LambertDiffuse(float3 kS, float3 albedo, float metalness)
{
    float3 kD = (float3(1.0f, 1.0f, 1.0f) - kS) * (1 - metalness);
    return (kD * albedo / PI);
}
//2.
float3 Diff_EnhanceLambertianModel(float3 Fresnel,float3 albeo)
{
    return (1 - Fresnel) * albeo / PI;
}
//3.
float3 Diff_SchlickFresnel(float3 F0,float albeo,float NdotL,float NdotV)
{
    float3 forward = 21 / (20.0 * PI) * (1 - F0) * albeo;
    float mid = 1 - pow((1 - max(NdotL,0.0f)), 5);
    float last = 1 - pow((1 - max(NdotV, 0.0f)), 5);

    return forward * mid * last;
}
//对于粗糙表面的模型
//1. Disney Diffuse

float3 Diffuse_Lambert(float3 DiffuseColor)
{
    return DiffuseColor * (1 / PI);
}

float3 Diff_DisneyDiffuse(float3 albeo,float roughness,float NdotV,float NdotL,float VdotH)
{
    float FD90 = 0.5 + 2 * VdotH * VdotH * roughness;
    float FdV = 1 + (FD90 - 1) * pow((1 - NdotV), 5);
    float FdL = 1 + (FD90 - 1) * pow((1 - NdotL), 5);
    return albeo * ((1 / PI) * FdV * FdL);
}


// [Gotanda 2014, "Designing Reflectance Models for New Consoles"]
float3 Diffuse_Gotanda(float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH)
{
    NoL = max(NoL, 0.0f);
    NoV = max(NoV, 0.0f);
  //  VoH = max(VoH, 0.0f);

    float a = Roughness * Roughness;
    float a2 = a * a;
    float F0 = 0.04;
    float VoL = 2 * VoH * VoH - 1; // double angle identity
 
    float Cosri = VoL - NoV * NoL;
#if 1
    float a2_13 = a2 + 1.36053;
    float Fr = (1 - (0.542026 * a2 + 0.303573 * a) / a2_13) * (1 - pow(1 - NoV, 5 - 4 * a2) / a2_13) * ((-0.733996 * a2 * a + 1.50912 * a2 - 1.16402 * a) * pow(1 - NoV, 1 + rcp(39 * a2 * a2 + 1)) + 1);
	//float Fr = ( 1 - 0.36 * a ) * ( 1 - pow( 1 - NoV, 5 - 4*a2 ) / a2_13 ) * ( -2.5 * Roughness * ( 1 - NoV ) + 1 );
    float Lm = (max(1 - 2 * a, 0) * (1 - pow((1 - NoL), 5)) + min(2 * a, 1)) * (1 - 0.5 * a * (NoL - 1)) * NoL;
    float Vd = (a2 / ((a2 + 0.09) * (1.31072 + 0.995584 * NoV))) * (1 - pow(1 - NoL, (1 - 0.3726732 * NoV * NoV) / (0.188566 + 0.38841 * NoV)));
    float Bp = Cosri < 0 ? 1.4 * NoV * NoL * Cosri : Cosri;
    float Lr = (21.0 / 20.0) * (1 - F0) * (Fr * Lm + Vd + Bp);
    return DiffuseColor / PI * Lr;
#else
	float a2_13 = a2 + 1.36053;
	float Fr = ( 1 - ( 0.542026*a2 + 0.303573*a ) / a2_13 ) * ( 1 - pow( 1 - NoV, 5 - 4*a2 ) / a2_13 ) * ( ( -0.733996*a2*a + 1.50912*a2 - 1.16402*a ) * pow( 1 - NoV, 1 + rcp(39*a2*a2+1) ) + 1 );
    float Lm = (max(1 - 2 * a, 0) * (1 - pow((1 - NoL), 5)) + min(2 * a, 1)) * (1 - 0.5 * a + 0.5 * a * NoL);
	float Vd = ( a2 / ( (a2 + 0.09) * (1.31072 + 0.995584 * NoV) ) ) * ( 1 - pow( 1 - NoL, ( 1 - 0.3726732 * NoV * NoV ) / ( 0.188566 + 0.38841 * NoV ) ) );
	float Bp = Cosri < 0 ? 1.4 * NoV * Cosri : Cosri / max( NoL, 1e-8 );
	float Lr = (21.0 / 20.0) * (1 - F0) * ( Fr * Lm + Vd + Bp );
	return DiffuseColor / PI * Lr;
#endif
}


