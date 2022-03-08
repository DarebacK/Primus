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

struct Input
{
	uint vertexIndex : SV_VertexID;
};

float4 vertexShaderMain(Input input) : SV_POSITION
{
	const float2 position = positions[input.vertexIndex];
	return mul(float4(position.x, 0.f, position.y, 1.f), transform);
}