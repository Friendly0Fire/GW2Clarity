#define CURSOR_HLSL
#include "common.hlsli"

float4 ColorFromDist(float l, float t) {
	if(l > t)
		discard;
	if(l < t - parameters.x)
		return color2;

	return color1;
}

float4 Circle(PS_INPUT In) : SV_Target
{
	float l = length(In.UV - 0.5f) * 2.f;
	return ColorFromDist(l, 1.f);
}

float4 Smooth(PS_INPUT In) : SV_Target
{
	float l = length(In.UV - 0.5f) * 2.f;
	return color1 * (1.f - smoothstep(saturate(parameters.x), 1.f, l));
}

float4 Square(PS_INPUT In) : SV_Target
{
	float l = max(abs(In.UV.x - 0.5f), abs(In.UV.y - 0.5f)) * 2.f;
	return ColorFromDist(l, 1.f);
}

float4 Cross(PS_INPUT In) : SV_Target
{
	float l1 = abs(cos(parameters.z) * (0.5f - In.UV.y) - sin(parameters.z) * (0.5f - In.UV.x));
	float l2 = abs(cos(parameters.z + PI * 0.5f) * (0.5f - In.UV.y) - sin(parameters.z + PI * 0.5f) * (0.5f - In.UV.x));
	return ColorFromDist(min(l1, l2), parameters.y);
}