#include "common.hlsli"

cbuffer Common : register(b0)
{
    float4 screenSize;
    float2 atlasUVSize;
    float2 numbersUVSize;
};

struct InstanceData
{
    float4 posDims;
    float2 uv;
    float2 numberUV;
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
	float4 Position    : SV_Position;
    float4 UV          : TEXCOORD0;

    nointerpolation float4 TexUVs      : TEXCOORD1;
    nointerpolation float4 Tint        : TEXCOORD3;
    nointerpolation float4 BorderColor : TEXCOORD4;
    nointerpolation float4 GlowColor   : TEXCOORD5;
    nointerpolation float3 BorderGlow  : TEXCOORD6;
    nointerpolation float  ShowNumber  : TEXCOORD7;
};

VS_OUT Grids_VS(in uint instance : SV_InstanceID, in uint id : SV_VertexID)
{
    VS_OUT Out = (VS_OUT)0;

    InstanceData data = Instances[instance];

    float2 UV = float2(id & 1, id >> 1);

	float2 expandedDims = data.posDims.zw + 2.f * data.glowSize * screenSize.zw;
    float2 dimsRatio = expandedDims / data.posDims.zw;
    float2 dimsExpansion = dimsRatio - 1.f;

    Out.UV.xy = UV * dimsRatio - 0.5f * dimsExpansion;
    Out.UV.zw = UV;
    Out.Position = float4((UV * 2 - 1) * expandedDims + data.posDims.xy * 2 - 1, 0.5f, 1.f);
	Out.Position.y *= -1;

    Out.TexUVs = float4(data.uv, data.numberUV);
    Out.Tint = data.tint;
    Out.BorderColor = data.borderColor;
    Out.GlowColor = data.glowColor;
    Out.ShowNumber = data.showNumber;
    Out.BorderGlow = float3(2.f * data.borderThickness / (data.posDims.zw * screenSize.xy), data.posDims.z * screenSize.x / data.glowSize);

    return Out;
}

float4 Grids_PS(in VS_OUT In) : SV_Target
{
    float2 threshold = abs(In.UV - 0.5f) * 2;
    bool inCore = all(threshold <= 1.f);
    float4 tex = inCore * Atlas.Sample(MainSampler, In.TexUVs.xy + In.UV.xy * atlasUVSize);
    float4 num = inCore * In.ShowNumber * Numbers.Sample(MainSampler, In.TexUVs.zw + In.UV.xy * numbersUVSize);

    float4 c = float4(lerp(tex.rgb, num.rgb, num.a), tex.a);
    c.rgb *= In.Tint.rgb;

    c += In.BorderColor * (inCore && any(threshold >= 1.f - In.BorderGlow.xy));
    c += In.GlowColor * saturate((1.f - c.a) - 2.f * length(In.UV.zw - 0.5f));

    c *= In.Tint.a;

    return c;
}