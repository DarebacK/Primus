#pragma pack_matrix(row_major)

Texture2D heightmap;
SamplerState heightmapSampler;

struct Input
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float4 pixelShaderMain(Input input) : SV_TARGET
{
	float grayscale = (heightmap.Sample(heightmapSampler, input.uv).r + 1.f) / 2.f;
	return float4(grayscale, grayscale, grayscale, 1.0f);
}