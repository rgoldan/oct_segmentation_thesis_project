#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

namespace oct {
	namespace proc {


		enum class ColormapType : int
		{
			Gray = 0,
			Sepia = 1,
			Green = 2,
		};
		
		struct ColormapRecord
		{
			DirectX::XMFLOAT4*	pLut;
			ColormapType		type;
		};


		class ColormapArrayResource
		{
		public:
			ColormapArrayResource(ID3D11Device *pDevice);
			~ColormapArrayResource();

			ID3D11ShaderResourceView* GetLutSRV() { return _pSRV; }

		protected:

			void DeleteColormaps();

			void InitColormap(DirectX::XMFLOAT4* pLut, int length, ColormapType type);
			void InitGray(DirectX::XMFLOAT4* pLut, int length);
			void InitGreen(DirectX::XMFLOAT4* pLut, int length);
			void InitRed(DirectX::XMFLOAT4* pLut, int length);
			void InitSepia(DirectX::XMFLOAT4* pLut, int length);

			//HRESULT CopyLutToGpu(ID3D11DeviceContext* pImmediateContext);

		protected:

			// GPU resources
			ID3D11Texture1D*			_pTex = NULL;
			ID3D11ShaderResourceView*	_pSRV = NULL;
			
			// CPU resources
			std::vector<ColormapRecord>	_colormap;
			
			int							_length = 0;
						
		};

	} // namespace proc
} // namespace oct
