#include "stdafx.h"
#include "PixelInfoResource.h"
#include "Utility.h"

namespace oct {
	namespace proc {

		PixelInfoResource::PixelInfoResource(ID3D11Device *pDevice)
		{
			HRESULT hr;

			PIXELINFO pixelInfo;
	
			D3D11_BUFFER_DESC desc;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.ByteWidth = sizeof(PIXELINFO);
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			desc.StructureByteStride = sizeof(PIXELINFO);

			D3D11_SUBRESOURCE_DATA initialData;
			initialData.pSysMem = &pixelInfo;
			initialData.SysMemPitch = 0;
			initialData.SysMemSlicePitch = 0;

			HR(pDevice->CreateBuffer(&desc, &initialData, &_pBuffer));


			// Read Buffer
			desc.Usage = D3D11_USAGE_STAGING;
			desc.BindFlags = 0;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			pDevice->CreateBuffer(&desc, &initialData, &_pReadBuffer);



			D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc;
			viewDesc.Format = DXGI_FORMAT_UNKNOWN;
			viewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			viewDesc.Buffer.FirstElement = 0;
			viewDesc.Buffer.NumElements = 1;
			//viewDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
			viewDesc.Buffer.Flags = 0;

			HR(pDevice->CreateUnorderedAccessView(_pBuffer, &viewDesc, &_pUAV));




			DXError:
				if (FAILED(hr))
				{
					SAFE_RELEASE(_pUAV);
					SAFE_RELEASE(_pBuffer);
					SAFE_RELEASE(_pReadBuffer);
				}

		}


		PixelInfoResource::~PixelInfoResource()
		{
			SAFE_RELEASE(_pUAV);
			SAFE_RELEASE(_pBuffer);
			SAFE_RELEASE(_pReadBuffer);
		}

		PIXELINFO PixelInfoResource::GetPixelInfo(ID3D11DeviceContext* pImmediateContext)
		{
			PIXELINFO pixelInfo;

			pImmediateContext->CopyResource(_pReadBuffer, _pBuffer);

			// Copy from the GPU
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			pImmediateContext->Map(_pReadBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);
			memcpy(&pixelInfo, mappedResource.pData, sizeof(PIXELINFO));
			pImmediateContext->Unmap(_pReadBuffer, 0);


			return pixelInfo;
		}

		//HRESULT LevelsResource::CopyToGpu(ID3D11DeviceContext* pImmediateContext, float black, float white)
		//{
		//	HRESULT hr;

		//	LEVELS levels = LEVELS(black, white);

		//	D3D11_MAPPED_SUBRESOURCE mappedResource;
		//	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

		//	// Disable GPU access 
		//	HR(pImmediateContext->Map(_pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

		//	// Copy data
		//	memcpy(mappedResource.pData, &levels, sizeof(LEVELS));

		//	// Reenable GPU accces
		//	pImmediateContext->Unmap(_pBuffer, 0);

		//DXError:
		//	return hr;
		//}

	} // namespace proc
} // namespace oct