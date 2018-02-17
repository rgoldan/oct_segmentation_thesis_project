#pragma once

#include <d3d11.h>
#include "Utility.h"
#include "OckFileSource.h"

namespace oct {
	namespace proc {

		class DataResource2D
		{
		public:
			DataResource2D(ID3D11Device *pDevice, UINT width, UINT height, DXGI_FORMAT format);
			~DataResource2D();

			UINT GetWidth()
			{
				return _desc.Width;
			}

			UINT GetHeight()
			{
				return _desc.Height;
			}

			DXGI_FORMAT GetFormat()
			{
				return _desc.Format;
			}

			ID3D11ShaderResourceView* GetShaderResourceView()
			{
				return _pSRV;
			}

			ID3D11Texture2D* GetTexture()
			{
				return _pTex;
			}

			HRESULT CopyToGpu(ID3D11DeviceContext* pImmediateContext, const void *pData, UINT bytePitch);

		private:

			D3D11_TEXTURE2D_DESC		_desc;
			ID3D11Texture2D*			_pTex = NULL;
			ID3D11ShaderResourceView*	_pSRV = NULL;
		};

	} // namespace proc
} // namespace oct

