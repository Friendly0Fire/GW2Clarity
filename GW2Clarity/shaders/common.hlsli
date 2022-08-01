#define PI 3.14159f
#define SQRT2 1.4142136f
#define ONE_OVER_SQRT2 0.707107f

#ifdef CURSOR_HLSL
cbuffer Cursor : register(b0)
{
	float4 dimensions;
	float4 parameters;
	float4 color1;
	float4 color2;
};
#endif

SamplerState MainSampler : register(s0);
SamplerState SecondarySampler : register(s1);

Texture2D<float4> MainTexture : register(t0);

struct PS_INPUT
{
	float4 pos : SV_Position;
	float2 UV : TEXCOORD0;
};

float2 rotate(float2 uv, float angle)
{
	float2x2 mat = float2x2(cos(angle), -sin(angle), sin(angle), cos(angle));
	return mul(uv, mat);
}

float rescale(float value, float2 bounds)
{
	return saturate((value - bounds.x) / (bounds.y - bounds.x));
}