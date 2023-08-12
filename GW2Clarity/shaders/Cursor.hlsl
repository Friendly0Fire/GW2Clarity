#include "common.hlsli"

cbuffer Cursor : register(b0)
{
    float4 dimensions;
    float4 parameters;
    float4 colorFill;
    float4 colorBorder;
}

#include "ScreenQuad.hlsli"

float4 ColorFromDist(float l, float t) {
    if(l > t)
        discard;
    if(l < t - parameters.x)
        return colorFill;

    return colorBorder;
}

float4 Circle_PS(PS_INPUT In) : SV_Target
{
    float l = length(In.UV - 0.5f) * 2.f;
    return ColorFromDist(l, 1.f);
}

float4 Square_PS(PS_INPUT In) : SV_Target
{
    float l = max(abs(In.UV.x - 0.5f), abs(In.UV.y - 0.5f)) * 2.f;
    return ColorFromDist(l, 1.f);
}

float4 Cross_PS(PS_INPUT In) : SV_Target
{
    float l1 = abs(cos(parameters.z) * (0.5f - In.UV.y) - sin(parameters.z) * (0.5f - In.UV.x));
    float l2 = abs(cos(parameters.z + PI * 0.5f) * (0.5f - In.UV.y) - sin(parameters.z + PI * 0.5f) * (0.5f - In.UV.x));
    return ColorFromDist(min(l1, l2), parameters.y);
}