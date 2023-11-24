#pragma pack_matrix(row_major)

cbuffer Transformation
{
	float4x4 transform;
};

struct Input
{
	float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

// TODO: get rid of this, if needed sample it in PS
Texture2D<min16int> heightmap;

struct Output
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float heightInMeters : HEIGHT;
};

Output vertexShaderMain(Input input)
{
	Output output;
	const float4 positionWorld = float4(input.position.x, input.position.y, input.position.z, 1.f);
	output.position = mul(positionWorld, transform);
	output.uv = input.uv;
    output.heightInMeters = float(heightmap.Load(int3(0, 0, 0)));
	return output;
}