#include"LightUtil.hlsl"

Texture2D gCurrentFrame : register(t0, space0);
Texture2D gHistoryBuffer:register(t0, space1);
Texture2D gUVVelicityBuffer:register(t0, space2);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);


float3 ClipAABB(float3 aabbMin, float3 aabbMax, float3 prevSample, float3 avg)
{
	// note: only clips towards aabb center (but fast!)
	float3 p_clip = 0.5 * (aabbMax + aabbMin);
	float3 e_clip = 0.5 * (aabbMax - aabbMin);

	float3 v_clip = prevSample - p_clip;
	float3 v_unit = v_clip.xyz / e_clip;
	float3 a_unit = abs(v_unit);
	float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

	if (ma_unit > 1.0)
		return p_clip + v_clip / ma_unit;
	else
		return prevSample;// point inside aabb

}

static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};


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

	float2 uvVelicity = gUVVelicityBuffer.Sample(gsamPointWrap, pin.TexC).rgb;
	float3 prevColor = gHistoryBuffer.Sample(gsamLinearWrap, pin.TexC+ uvVelicity).rgb;
	//需要把屏幕长宽传进来
	float dWidth = 1 / 1000;
	float dHeight = 1 / 750;
	// Variance clip.
	float3 m1 = float3(0.0f, 0.0f, 0.0f);
	float3 m2 = float3(0.0f, 0.0f, 0.0f);
	for (int i=-1;i<2;i++)
	{
		for (int j = -1; j < 2; j++)
		{
			 m1 += gCurrentFrame.Sample(gsamLinearWrap, pin.TexC+float2(dWidth*i,dHeight*j)).rgb;
			 m2 += m1 * m1;
		}
	}
	
	float3 mu = m1 / 9;
	float3 sigma = sqrt(abs(m2 / 9 - mu * mu));
	float3 minc = mu - 1 * sigma;
	float3 maxc = mu + 1 * sigma;




	prevColor = ClipAABB(minc, maxc, prevColor, mu);

	mu = lerp(mu, prevColor,0.5f);
	return float4(mu, 1.0f);
//	return float4(1.0f,1.0f,1.0f, 1.0f);
}
