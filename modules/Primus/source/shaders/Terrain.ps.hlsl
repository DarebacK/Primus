#pragma pack_matrix(row_major)

struct Input
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	min16int height : HEIGHT;
};

float4 pixelShaderMain(Input input) : SV_TARGET
{
	if (input.height < 0)
	{
		discard;
	}
	return float4(input.uv.x, input.uv.y, 0.f, 1.0f);
}