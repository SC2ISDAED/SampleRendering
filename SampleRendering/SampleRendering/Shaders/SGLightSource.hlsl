#define PI 3.1415926
struct SG
{
    float3 Amplitude;
    float3 Axis;
    float3 Sharpness;
};
//
float3 EvaluateSG(SG sg, float3 dir)
{
    float cosAngle = dot(dir, sg.Axis);

    return sg.Amplitude * exp(sg.Sharpness * (cosAngle - 1.0f));
}
//SG积分计算
float3 SGIntegral(SG sg)
{
    float expTerm = 1.0f - exp(-2.0f * sg.Sharpness);
    return 2
    * PI * (sg.Amplitude / sg.Sharpness) * expTerm;
}
float3 SGInnerProduct(in SG x, in SG y)
{
    float umLength = length(x.Sharpness * x.Axis +
                            y.Sharpness * y.Axis);
    float3 expo = exp(umLength - x.Sharpness - y.Sharpness) *
                  x.Amplitude * y.Amplitude;
    float other = 1.0f - exp(-2.0f * umLength);
    return (2.0f * PI * expo * other) / umLength;
}


float3 ApproximateSGintegral(SG sg)
{
    return 2 * PI * (sg.Amplitude / sg.Sharpness);
}

float SGSharpnessFromThreshold(float amplitude, float epsilon, float cosTheta)
{
    return (log(epsilon) - log(amplitude)) / (cosTheta - 1.0f);
}
SG CosineLobeSG(float3 direction)
{
    SG cosineLobe;
    cosineLobe.Axis = direction;
    cosineLobe.Sharpness = 2.133f;
    cosineLobe.Amplitude = 1.17f;

    return cosineLobe;
}

float3 SGIrradianceInnerProduct(SG lightingLobe, float3 normal)
{
    SG cosineLobe = CosineLobeSG(normal);
    return max(SGInnerProduct(lightingLobe, cosineLobe), 0.0f);
}
//将cosin 固定到积分外，降低耗费
float3 SGIrradiancePunctual(in SG lightingLobe, in float3 normal)
{
    float cosineTerm = saturate(dot(lightingLobe.Axis, normal));
    return cosineTerm * 2.0f * PI * (lightingLobe.Amplitude) /
                                     lightingLobe.Sharpness;
}
//更精确的内积近似
float3 SGIrradianceFitted(in SG lightingLobe, in float3 normal)
{
    lightingLobe.Sharpness = 4.0f;
    lightingLobe.Amplitude = 0.5f;
    const float muDotN = dot(lightingLobe.Axis, normal);
    const float lambda = lightingLobe.Sharpness;
 
    const float c0 = 0.36f;
    const float c1 = 1.0f / (4.0f * c0);
 
    float eml = exp(-lambda);
    float em2l = eml * eml;
    float rl = rcp(lambda);
 
    float scale = 1.0f + 2.0f * em2l - rl;
    float bias = (eml - em2l) * rl - em2l;
 
    float x = sqrt(1.0f - scale);
    float x0 = c0 * muDotN;
    float x1 = c1 * x;
 
    float n = x0 + x1;
 
    float y = saturate(muDotN);
    if (abs(x0) <= x1)
        y = n * n / x;
 
    float result = scale * y + bias;
 
    return result * SGIntegral(lightingLobe);
}

float3 SGDiffuseInnerProduct(SG lightingLobe, in float3 normal,
                             in float3 albedo,float3 BRDF)
{
    float3 brdf = BRDF;
    return SGIrradianceFitted(lightingLobe, normal) * brdf;
}
//将SG lobe 转到light libe 相同的域下
//也就是将 圆域转换到 half vector 的 45° 域下
SG WarpDistributionSG(in SG ndf, in float3 view)
{
    SG warp;
 
    warp.Axis = reflect(-view, ndf.Axis);
    warp.Amplitude = ndf.Amplitude;
    warp.Sharpness = ndf.Sharpness;
    warp.Sharpness /= (4.0f * max(dot(ndf.Axis, view), 0.0001f));
 
    return warp;
}
SG DistributionTermSG(in float3 direction, in float roughness)
{
    SG distribution;
    distribution.Axis = direction;
    float m2 = roughness * roughness;
    distribution.Sharpness = 2 / m2;
    distribution.Amplitude = 1.0f / (PI * m2);
 
    return distribution;
}
float GGX_V1(in float m2, in float nDotX)
{
    return 1.0f / (nDotX + sqrt(m2 + (1 - m2) * nDotX * nDotX));
}
 
float3 SpecularTermSGWarp(in SG light, in float3 normal,
                          in float roughness, in float3 view,
                          in float3 specAlbedo)
{
    // Create an SG that approximates the NDF.
    SG ndf = DistributionTermSG(normal, roughness);
 
    // Warp the distribution so that our lobe is in the same
    // domain as the lighting lobe
    SG warpedNDF = WarpDistributionSG(ndf, view);
 
     // Convolve the NDF with the SG light
    float3 output = SGInnerProduct(warpedNDF, light);
 
    // Parameters needed for the visibility
    float3 warpDir = warpedNDF.Axis;
    float m2 = roughness * roughness;
    float nDotL = saturate(dot(normal, warpDir));
    float nDotV = saturate(dot(normal, view));
    float3 h = normalize(warpedNDF.Axis + view);
 
    // Visibility term evaluated at the center of
    // our warped BRDF lobe
    output *= GGX_V1(m2, nDotL) * GGX_V1(m2, nDotV);
 
    // Fresnel evaluated at the center of our warped BRDF lobe
    float powTerm = pow((1.0f - saturate(dot(warpDir, h))), 5);
    output *= specAlbedo + (1.0f - specAlbedo) * powTerm;
 
    // Cosine term evaluated at the center of the BRDF lobe
    output *= nDotL;
 
    return max(output, 0.0f);
}
//各向异性的 SG 高光
struct ASG
{
    float3 Amplitude;
    float3 BasisZ;
    float3 BasisX;
    float3 BasisY;
    float SharpnessX;
    float SharpnessY;
};

float3 EvaluateASG(in ASG asg, in float3 dir)
{
    float sTerm = saturate(dot(asg.BasisZ, dir));
    float lambdaTerm = asg.SharpnessX * dot(dir, asg.BasisX)
                                      * dot(dir, asg.BasisX);
    float muTerm = asg.SharpnessY * dot(dir, asg.BasisY)
                                  * dot(dir, asg.BasisY);
    return asg.Amplitude * sTerm * exp(-lambdaTerm - muTerm);
}
float3 ConvolveASG_SG(in ASG asg, in SG sg)
{
    // The ASG paper specifes an isotropic SG as
    // exp(2 * nu * (dot(v, axis) - 1)),
    // so we must divide our SG sharpness by 2 in order
    // to get the nup parameter expected by the ASG formula
    float nu = sg.Sharpness * 0.5f;
 
    ASG convolveASG;
    convolveASG.BasisX = asg.BasisX;
    convolveASG.BasisY = asg.BasisY;
    convolveASG.BasisZ = asg.BasisZ;
 
    convolveASG.SharpnessX = (nu * asg.SharpnessX) /
                             (nu + asg.SharpnessX);
    convolveASG.SharpnessY = (nu * asg.SharpnessY) /
                             (nu + asg.SharpnessY);
 
    convolveASG.Amplitude = PI / sqrt((nu + asg.SharpnessX) *
                                 (nu + asg.SharpnessY));
 
    float3 asgResult = EvaluateASG(convolveASG, sg.Axis);
    return asgResult * sg.Amplitude * asg.Amplitude;
}
 
ASG WarpDistributionASG(in SG ndf, in float3 view)
{
    ASG warp;
 
    // Generate any orthonormal basis with Z pointing in the
    // direction of the reflected view vector
    warp.BasisZ = reflect(-view, ndf.Axis);
    warp.BasisX = normalize(cross(ndf.Axis, warp.BasisZ));
    warp.BasisY = normalize(cross(warp.BasisZ, warp.BasisX));
 
    float dotDirO = max(dot(view, ndf.Axis), 0.0001f);
 
    // Second derivative of the sharpness with respect to how
    // far we are from basis Axis direction
    warp.SharpnessX = ndf.Sharpness / (8.0f * dotDirO * dotDirO);
    warp.SharpnessY = ndf.Sharpness / 8.0f;
 
    warp.Amplitude = ndf.Amplitude;
 
    return warp;
}
 
float3 SpecularTermASGWarp(in SG light, in float3 normal,
                           in float roughness, in float3 view,
                           in float3 specAlbedo)
{
    // Create an SG that approximates the NDF
    SG ndf = DistributionTermSG(normal, roughness);
 
    // Apply a warpring operation that will bring the SG from
    // the half-angle domain the the the lighting domain.
    ASG warpedNDF = WarpDistributionASG(ndf, view);
 
    // Convolve the NDF with the light
    float3 output = ConvolveASG_SG(warpedNDF, light);
 
    // Parameters needed for evaluating the visibility term
    float3 warpDir = warpedNDF.BasisZ;
    float m2 = roughness * roughness;
    float nDotL = saturate(dot(normal, warpDir));
    float nDotV = saturate(dot(normal, view));
    float3 h = normalize(warpDir + view);
 
    // Visibility term
    output *= GGX_V1(m2, nDotL) * GGX_V1(m2, nDotV);
 
    // Fresnel
    float powTerm = pow((1.0f - saturate(dot(warpDir, h))), 5);
    output *= specAlbedo + (1.0f - specAlbedo) * powTerm;
 
    // Cosine term
    output *= nDotL;
 
    return max(output, 0.0f);
}