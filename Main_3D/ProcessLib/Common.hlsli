
struct VS_INPUT
{
	float2 Pos : POSITION;
	float2 Tex : TEXCOORD;
};


struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD;
};
