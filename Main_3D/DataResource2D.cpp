#include "stdafx.h"
#include "DataResource2D.h"

namespace oct {
	namespace proc {
		
		DataResource2D::DataResource2D(ID3D11Device *pDevice, UINT width, UINT height, DXGI_FORMAT format)
		{

			HRESULT hr;
			DXGI_SAMPLE_DESC sampleDescription = { 1, 0 }; // No multisampling

			ZeroMemory(&_desc, sizeof(D3D11_TEXTURE2D_DESC));
			_desc.Width = width;
			_desc.Height = height;
			_desc.MipLevels = 1;
			_desc.ArraySize = 1;
			_desc.Format = format;
			_desc.SampleDesc = sampleDescription;
			_desc.Usage = D3D11_USAGE_DYNAMIC;
			_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			_desc.MiscFlags = 0;

			HR(pDevice->CreateTexture2D(&_desc, NULL, &_pTex));
			HR(pDevice->CreateShaderResourceView(_pTex, NULL, &_pSRV));

		DXError:

			if (FAILED(hr))
			{
				SAFE_RELEASE(_pTex);
				SAFE_RELEASE(_pSRV);
			}
		}


		DataResource2D::~DataResource2D()
		{
			SAFE_RELEASE(_pTex);
			SAFE_RELEASE(_pSRV);
		}

		HRESULT DataResource2D::CopyToGpu(ID3D11DeviceContext* pImmediateContext, const void *pSrc, UINT srcBytePitch)
		{
			HRESULT hr;
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			IppiSize roiSize = { (int)_desc.Width, (int)_desc.Height };

			// Disable GPU access 
			HR(pImmediateContext->Map(_pTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

			// Copy data
			// The runtime might assign values to RowPitch and DepthPitch that are larger than anticipated because there might be padding between rows and depth
			switch (_desc.Format)
			{
			case DXGI_FORMAT_R32_FLOAT:
				ippiCopy_32f_C1R((Ipp32f*)pSrc, srcBytePitch, (Ipp32f*)mappedResource.pData, mappedResource.RowPitch, roiSize);
				break;
			case DXGI_FORMAT_R16_SINT:
			case DXGI_FORMAT_R16_SNORM:
				ippiCopy_16s_C1R((Ipp16s*)pSrc, srcBytePitch, (Ipp16s*)mappedResource.pData, mappedResource.RowPitch, roiSize);
				break;
			default:
				assert(false);
			}

			// Reenable GPU accces
			pImmediateContext->Unmap(_pTex, 0);	

		DXError:
			return hr;
		}

	} // namespace proc
} // namespace oct