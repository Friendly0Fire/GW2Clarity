#define CURSOR_HLSL
#include "common.hlsli"

struct VS_SCREEN
{
	float4 Position : SV_Position;
	float2 UV : TEXCOORD0;
};

VS_SCREEN ScreenQuad(in uint id : SV_VertexID)
{
    VS_SCREEN Out = (VS_SCREEN)0;

	float2 UV = float2(id & 1, id >> 1);

	float2 dims = (UV * 2 - 1) * dimensions.zw;

    Out.UV = UV;
    Out.Position = float4(dims + dimensions.xy * 2 - 1, 0.5f, 1.f);
	Out.Position.y *= -1;

    return Out;
}