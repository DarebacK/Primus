#pragma pack_matrix(row_major)

cbuffer Transformation
{
	float4x4 transform;
};

static float2 positions[] =
{
	float2( 0.f,  0.f),
	float2( 0.f,  1.f),
	float2( 1.f,  1.f),
	float2( 1.f,  0.f),
};

static float2 uvs[] =
{
	float2(0.f,  1.f),
	float2(0.f,  0.f),
	float2(1.f,  0.f),
	float2(1.f,  1.f),
};

struct Input
{
	uint vertexIndex : SV_VertexID;
};

struct Output
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

Output vertexShaderMain(Input input)
{
	const float2 positionInWorld = positions[input.vertexIndex];
	Output output;
	output.position = mul(float4(positionInWorld.x, 0.f, positionInWorld.y, 1.f), transform);
	output.uv = uvs[input.vertexIndex];
	return output;
}