#pragma pack_matrix(row_major)

struct Input
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float heightInMeters : HEIGHT;
};

Texture2D colormap;
SamplerState colormapSampler;

static const float4 deepestWaterColor = { 0.f, 0.f, 0.f, 1.f };
static const float4 shallowestWaterColor = { 0.12549019607f, 0.2862745098f, 0.29019607843f, 1.f };
static const float deepestWaterHeight = -11000.f;
static const float shallowestWaterHeight = 0.f;

float4 pixelShaderMain(Input input) : SV_TARGET
{
	const float t = step(0.f, input.heightInMeters);
	const float4 waterColor = lerp(deepestWaterColor, shallowestWaterColor, smoothstep(deepestWaterHeight, shallowestWaterHeight, input.heightInMeters));
	const float4 landColor = colormap.Sample(colormapSampler, input.uv);
	return lerp(waterColor, landColor, t);
}