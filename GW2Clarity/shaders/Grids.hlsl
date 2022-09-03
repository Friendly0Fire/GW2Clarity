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
	float4 Position    : SV_Position;
    float4 UV          : TEXCOORD0;

    float4 TexUV       : TEXCOORD1;
    float4 NumUV       : TEXCOORD2;
    float4 Tint        : TEXCOORD3;
    float4 BorderColor : TEXCOORD4;
    float4 GlowColor   : TEXCOORD5;
    float3 BorderGlow  : TEXCOORD6;
    float  ShowNumber  : TEXCOORD7;
};

VS_OUT Grids_VS(in uint instance : SV_InstanceID, in uint id : SV_VertexID)
{
    VS_OUT Out = (VS_OUT)0;

    InstanceData data = Instances[instance];

    float2 UV = float2(id & 1, id >> 1);

	float2 dims = data.posDims.zw + 2.f * data.glowSize * screenSize.zw;

    Out.UV.xy = UV * (dims / data.posDims.zw) - 0.5f * ((dims / data.posDims.zw) - 1.f);
    Out.UV.zw = UV;
    Out.Position = float4((UV * 2 - 1) * dims + data.posDims.xy * 2 - 1, 0.5f, 1.f);
	Out.Position.y *= -1;

    Out.TexUV = data.uv;
    Out.NumUV = data.numberUV;
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
    float4 tex = inCore * Atlas.Sample(MainSampler, In.TexUV.xy + In.UV.xy * (In.TexUV.zw - In.TexUV.xy));
    float4 num = inCore * In.ShowNumber * Numbers.Sample(MainSampler, In.NumUV.xy + In.UV.xy * (In.NumUV.zw - In.NumUV.xy));

    float4 c = float4(lerp(tex.rgb, num.rgb, num.a), tex.a);
    c.rgb *= In.Tint.rgb;

    c += In.BorderColor * (inCore && any(threshold >= 1.f - In.BorderGlow.xy));
    c += In.GlowColor * saturate((1.f - c.a) - 2.f * length(In.UV.zw - 0.5f));

    c *= In.Tint.a;

    return c;
}