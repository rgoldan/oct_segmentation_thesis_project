#include "stdafx.h"
#include "ColormapArrayResource.h"
#include "Utility.h"
#include "resource.h"

namespace oct {
	namespace proc {

		ColormapArrayResource::ColormapArrayResource(ID3D11Device *pDevice)
		{

			HRESULT hr;

			// All colormaps will be 256 in length
			_length = 256;
			
			// Create colormaps
			const int n1 = static_cast<int>(ColormapType::Gray);	// First
			const int n2 = static_cast<int>(ColormapType::Green);	// Last
			const int colormapCount = n2 - n1 + 1;
			
			ColormapRecord cr;
			D3D11_SUBRESOURCE_DATA texData[colormapCount];

			for (int n = n1; n < colormapCount; n++)
			{
				// Add colormap record
				cr.type = static_cast<ColormapType>(n);
				cr.pLut = new DirectX::XMFLOAT4[_length];
				_colormap.push_back(cr);

				// Init colormap
				InitColormap(cr.pLut, _length, cr.type);

				// Init D3D11_SUBRESOURCE_DATA
				ZeroMemory(&texData[n], sizeof(D3D11_SUBRESOURCE_DATA));
				texData[n].pSysMem = cr.pLut;
				texData[n].SysMemPitch = _length * sizeof(DirectX::XMFLOAT4);
				texData[n].SysMemSlicePitch = 0;

			}

			
			// Create Texture array
			D3D11_TEXTURE1D_DESC texDesc;
			ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE1D_DESC));
			texDesc.Width = _length;
			texDesc.MipLevels = 1;
			texDesc.ArraySize = colormapCount;
			texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			texDesc.Usage = D3D11_USAGE_IMMUTABLE;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;


			HR(pDevice->CreateTexture1D(&texDesc, texData, &_pTex));
			HR(pDevice->CreateShaderResourceView(_pTex, NULL, &_pSRV));

			DXError:
			
			if (FAILED(hr))
				{
					SAFE_RELEASE(_pTex);
					SAFE_RELEASE(_pSRV);
					DeleteColormaps();
				}
		}


		ColormapArrayResource::~ColormapArrayResource()
		{
			SAFE_RELEASE(_pTex);
			SAFE_RELEASE(_pSRV);
			DeleteColormaps();
		}

		void ColormapArrayResource::DeleteColormaps()
		{
			for (std::vector<ColormapRecord>::iterator it = _colormap.begin(); it != _colormap.end(); ++it)
			{
				SAFE_DELETE_ARRAY(it->pLut)
			}
		}

		// Protected Functions
		
		void ColormapArrayResource::InitColormap(DirectX::XMFLOAT4* pLut, int length, ColormapType type)
		{
			
			// Build the lut
			switch (type)
			{
			case ColormapType::Gray:
				InitGray(pLut, length);
				break;
			case ColormapType::Sepia:
				InitRed(pLut, length);
				break;
			case ColormapType::Green:
				InitGreen(pLut, length);
				break;
			default:
				assert("No init fuction for this colormap.");

			}

		}
		

		void ColormapArrayResource::InitGray(DirectX::XMFLOAT4* pLut, int length)
		{
			for (int n = 0; n < length; n++)
			{
				float v = (float)n / (length - 1);
				pLut[n] = DirectX::XMFLOAT4(v, v, v, 1.0);			// (R,G,B,A)
			}
		}

		void ColormapArrayResource::InitGreen(DirectX::XMFLOAT4* pLut, int length)
		{
			for (int n = 0; n < length; n++)
			{
				float v = (float)n / (length - 1);
				pLut[n] = DirectX::XMFLOAT4(0.0, v, 0.0, 1.0);		// (R,G,B,A)
			}
		}

		void ColormapArrayResource::InitRed(DirectX::XMFLOAT4* pLut, int length)
		{
			for (int n = 0; n < length; n++)
			{
				float v = (float)n / (length - 1);
				pLut[n] = DirectX::XMFLOAT4(v, 0.0, 0.0, 1.0);		// (R,G,B,A)
			}
		}

		void ColormapArrayResource::InitSepia(DirectX::XMFLOAT4* pLut, int length)
		{
			union BGRAVALUE
			{
				struct
				{
					unsigned char Blue;
					unsigned char Green;
					unsigned char Red;
					unsigned char Alpha;
				};
				unsigned int Value;
			};

			//// Sepia
			//HRSRC colormap = ::FindResource(NULL, MAKEINTRESOURCE(IDR_COLORMAP_SEPIA), "COLORMAP");
			//assert(colormap);
			//// The COLORMAP resource must be linked into the final execulatable or DLL
			//// Adding the staic library as a reference will not do this
			//// Add ProcessLib.res as a dependency in the linker/additional dependencies 
			////	"..\ProcessLib\x64\Debug\ProcessLib.res"
			////  "..\ProcessLib\x64\Release\ProcessLib.res"

			//// Only length 256 tables are supported
			//assert(::SizeofResource(NULL, colormap) == 4 * _length);

			//// Load the map
			//HGLOBAL hData = ::LoadResource(NULL, colormap);
			//BGRAVALUE*	pSepia = (BGRAVALUE*)::LockResource(hData);

			//for (int n = 0; n < _length; n++)
			//{
			//	_pLut[n].x = float(pSepia[n].Blue / 255.0);
			//	_pLut[n].y = float(pSepia[n].Green / 255.0);
			//	_pLut[n].z = float(pSepia[n].Red / 255.0);
			//	_pLut[n].w = float(pSepia[n].Alpha / 255.0);
			//}

		}


	} // namespace proc
} // namespace oct