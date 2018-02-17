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


float4 main(VS_OUTPUT input) : SV_Target
{
	// Interpolate texture to get value
	float value = gData.Sample(gDataSampler, input.Tex);

	// Normalize [Black <= value <= white] -> [0.0 <= value <= 1.0]
	float valueNorm = (value - Black) / Contrast;

	// Interpolate value from colormap
	float2 location = float2(valueNorm, Colormap);
	float4 color = gColormap.Sample(gColormapSampler, location);


return color;

}