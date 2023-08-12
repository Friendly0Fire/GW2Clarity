#include "common.hlsli"

static const float4 dimensions = float4(0.5f, 0.5f, 1, 1);

#include "ScreenQuad.hlsli"

float4 Copy_PS(in VS_SCREEN vs) : SV_Target0
{
    return MainTexture.SampleLevel(MainSampler, vs.UV, 0.f);
}