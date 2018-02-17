#include "Common.hlsli"

// Constant buffer
cbuffer T : register(b0)
{
	// Levels
	float Black;
	float White;
	float Contrast;

	// Colormap
	int Colormap;

	// ALine range
	float Rho1;
	float Rho2;
	float RhoDelta;

	// Padding
	float Reserved[2];
}

// Textures
Texture1DArray <float4> gColormap : register(t0);
Texture2D <float> gData : register(t1);

// Smaplers
SamplerState gColormapSampler : register(s0);
SamplerState gDataSampler : register(s1);


static const float gPI = 3.14159265f;
static const float g2PI = 2*gPI;


float4 main(VS_OUTPUT input) : SV_Target
{
	// Normalize texture coordinates on (-1 1)
	float2 normPos = 2 * input.Tex - 1.0;

	// Cartesian to polar transformation
	float rho = length(normPos);				// 0<rho<1
	float theta = atan2(normPos.y, normPos.x);	// -PI<theta<PI

	// Rescale rho to apply offset
	rho = rho * RhoDelta + Rho1;				// rho1<rho<rho2

	// Normalize theta as  texture coordinate
	theta = (theta + gPI) / g2PI;				// 0<theta<1
	
	// Interpolate texture to get pixrl value
	float2 pos = float2(rho, theta);
	float value = gData.Sample(gDataSampler, pos);

	// Normalize [Black <= value <= white] -> [0.0 <= value <= 1.0]
	float valueNorm = (value - Black) / Contrast;

	// Interpolate value from colormap
	float2 location = float2(valueNorm, Colormap);
	float4 color = gColormap.Sample(gColormapSampler, location);


return color;

}