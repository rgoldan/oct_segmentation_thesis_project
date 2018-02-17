
#include "Common.hlsli"


VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	// pos(x,y) is in already in homogenous clip space (-1<=x<=1,-1<=y<=1)
	output.Pos = float4(input.Pos.x, input.Pos.y, 1.0, 1.0);
	output.Tex = input.Tex;

	return output;
}