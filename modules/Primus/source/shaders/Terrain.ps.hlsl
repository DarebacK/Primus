#pragma pack_matrix(row_major)

struct Input
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float height : HEIGHT;
};

static const float4 deepestWaterColor = { 0.f, 0.f, 0.f, 1.f };
static const float4 shallowestWaterColor = { 0.12549019607f, 0.2862745098f, 0.29019607843f, 1.f };
static const min16int deepestWaterHeight = -11000;
static const min16int shallowestWaterHeight = 0;

float4 pixelShaderMain(Input input) : SV_TARGET
{
	if (input.height < 0.f)
	{
		return lerp(deepestWaterColor, shallowestWaterColor, smoothstep(float(deepestWaterHeight), float(shallowestWaterHeight), input.height));
	}
	else
	{
		return float4(input.uv.x, input.uv.y, 0.f, 1.0f);
	}
}