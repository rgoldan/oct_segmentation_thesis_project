#include "Common.hlsli"
#include "D3DX_DXGIFormatConvert.inl"

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

// Pixel Info
struct PixelInfo
{
	int X;
	int Y;
	int brga;
	float value;
};

// Textures
Texture1DArray <float4> gColormap : register(t0);
Texture2D <float> gData : register(t1);

// Smaplers
SamplerState gColormapSampler : register(s0);
SamplerState gDataSampler : register(s1);

RWStructuredBuffer<PixelInfo> pixelInfo	: register(u1);

void main(VS_OUTPUT input)
{
	// Interpolate texture to get value
	float value = gData.Sample(gDataSampler, input.Tex);

	// Normalize [Black <= value <= white] -> [0.0 <= value <= 1.0]
	float valueNorm = (value - Black) / Contrast;

	// Interpolate value from colormap
	float2 location = float2(valueNorm, Colormap);
	float4 color = gColormap.Sample(gColormapSampler, location);


	pixelInfo[0].X = int(input.Pos[0]);
	pixelInfo[0].Y = int(input.Pos[1]);

	pixelInfo[0].brga = D3DX_FLOAT4_to_B8G8R8A8_UNORM(color);
	pixelInfo[0].value = value;

}