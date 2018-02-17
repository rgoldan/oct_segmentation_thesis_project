#pragma once

#include <d3d11.h>
#include <DirectXMath.h>


namespace oct {
	namespace proc {

		struct PIXELINFO
		{
			int X;
			int Y;
			int brga;
			float value;
			//char reserved[];		// pad to 16 bytes

		public:
			//LEVELS() : _black(0.0), _white(1.0), _delta(1.0), _reserved(0.0) {}
			//LEVELS(float black, float white) : _black(black), _white(white), _delta(white-black), _reserved(0.0) {}
			//float GetBlack() { return _black; }
			//float GetWhite() { return _white; }
			//void SetBlack(float black) { _black = black; _delta = _white - _black; }
			//void SetWhite(float white) { _white = white; _delta = _white - _black; }
		};

		class PixelInfoResource
		{
			


		public:
			PixelInfoResource(ID3D11Device *pDevice);
			~PixelInfoResource();

			ID3D11Buffer* GetBuffer()
			{
				return _pBuffer;
			}

			ID3D11UnorderedAccessView* GetUAV()
			{
				return _pUAV;
			}

			PIXELINFO GetPixelInfo(ID3D11DeviceContext* pImmediateContext);

			//HRESULT CopyToGpu(ID3D11DeviceContext* pImmediateContext, float black, float white);

		private:

			ID3D11Buffer* _pBuffer = NULL;
			ID3D11Buffer* _pReadBuffer = NULL;
			ID3D11UnorderedAccessView* _pUAV = NULL;
		};

	} // namespace proc
} // namespace oct
