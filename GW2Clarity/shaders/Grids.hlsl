#include "common.hlsli"

struct InstanceData
{
    float4 posDims;
    float4 uv;
    float4 numberUV;
    float4 tint;
    float4 borderColor;
    float4 glowColor;
    float borderThickness;
    float glowSize;
    bool showNumber;
    int _;
};

StructuredBuffer<InstanceData> Instances : register(t0);
Texture2D<float4> Atlas : register(t1);
Texture2D<float4> Numbers : register(t2);

struct VS_OUT
{
	float4 Position : SV_Position;
    float2 UV : TEXCOORD0;

    float4 TexUV       : TEXCOORD1;
    float4 NumUV       : TEXCOORD2;
    float4 Tint        : TEXCOORD3;
    float4 BorderColor : TEXCOORD4;
    float4 GlowColor   : TEXCOORD5;
    float3 Data        : TEXCOORD6;
};

VS_OUT Grids_VS(in int instance : SV_InstanceID, in int id : SV_VertexID)
{
    VS_OUT Out = (VS_OUT)0;

    InstanceData data = Instances[instance];

    float2 UV = float2(id & 1, id >> 1);

	float2 dims = (UV * 2 - 1) * data.posDims.zw;

    Out.UV = UV;
    Out.Position = float4(dims + data.posDims.xy * 2 - 1, 0.5f, 1.f);
	Out.Position.y *= -1;

    Out.TexUV = data.uv;
    Out.NumUV = data.numberUV;
    Out.Tint = data.tint;
    Out.BorderColor = data.borderColor;
    Out.GlowColor = data.glowColor;
    Out.Data = float3(data.borderThickness, data.glowSize, data.showNumber);

    return Out;
}

float4 Grids_PS(in VS_OUT In) : SV_Target
{
    return 1.f;
}