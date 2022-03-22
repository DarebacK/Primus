#pragma pack_matrix(row_major)

cbuffer Transformation
{
	float4x4 transform;
};

static const uint heightmapWidth = 2560;
static const uint heightmapHeight = 3072;

struct Input
{
	uint vertexIndex : SV_VertexID;
};

Texture2D<min16int> heightmap;

struct Output
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float height : HEIGHT;
};

Output vertexShaderMain(Input input)
{
	const uint x = input.vertexIndex % heightmapWidth;
	const uint y = input.vertexIndex / heightmapWidth;

	const float height = float(heightmap.Load(int3(x, y, 0)));

	Output output;
	output.uv.x = x / float(heightmapWidth - 1);
	output.uv.y = y / float(heightmapHeight - 1);
	const float4 positionWorld = float4(output.uv.x, max(height, 0.f), 1.f - output.uv.y, 1.f);
	output.position = mul(positionWorld, transform);
	output.height = float(height);
	return output;
}