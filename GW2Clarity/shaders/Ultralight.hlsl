#include "common.hlsli"

float4 Ultralight(PS_INPUT In) : SV_Target
{
    return MainTexture.SampleLevel(MainSampler, In.UV, 0.f);
}