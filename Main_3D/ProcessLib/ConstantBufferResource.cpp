#include "stdafx.h"
#include "ConstantBufferResource.h"
#include "Utility.h"
#include "resource.h"

namespace oct {
	namespace proc {

		ConstantBufferResource::ConstantBufferResource(ID3D11Device *pDevice, float black, float white, ColormapType colormap)
		{

			HRESULT hr;

			// Levels
			_data.black = black;
			_data.white = white;
			_data.contrast = _data.white - _data.black;

			// Colormap
			_data.colormap = static_cast<int>(colormap);

			// ALine Range
			_data.rho1 = 0.0;
			_data.rho2 = 1.0;
			_data.deltaRho = _data.rho2 - _data.rho1;
			
			// Save a copy for default values
			_defaultData = _data;
						
			D3D11_BUFFER_DESC bufDesc;
			bufDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufDesc.ByteWidth = sizeof(CONSTANTBUFFERDATA);
			bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufDesc.MiscFlags = 0;
			bufDesc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA bufData;
			bufData.pSysMem = &_data;
			bufData.SysMemPitch = 0;
			bufData.SysMemSlicePitch = 0;

			HR(pDevice->CreateBuffer(&bufDesc, &bufData, &_pBuffer));
			_commitRequired = false;



			DXError:
				if (FAILED(hr))
				{
					SAFE_RELEASE(_pBuffer);
				}
		}


		ConstantBufferResource::~ConstantBufferResource()
		{
			SAFE_RELEASE(_pBuffer);
		}


		void ConstantBufferResource::ResetLevels()
		{
			_data.black = _defaultData.black;
			_data.white = _defaultData.white;
			_data.contrast = _data.white - _data.black;
			_commitRequired = true;
		}


		void ConstantBufferResource::SetRho1(float value)
		{
			_data.rho1 = value;
			_data.deltaRho = _data.rho2 - _data.rho1;
			_commitRequired = true;
		}

		void ConstantBufferResource::SetBlackLevel(float value)
		{
			_data.black = value;
			_data.contrast = _data.white - _data.black;
			_commitRequired = true;
		}

		void ConstantBufferResource::SetWhiteLevel(float value)
		{
			_data.white = value;
			_data.contrast = _data.white - _data.black;
			_commitRequired = true;
		}

		void ConstantBufferResource::SetColormap(ColormapType value)
		{
			_data.colormap = static_cast<int>(value);
			_commitRequired = true;
		}

		void ConstantBufferResource::ResetColormap()
		{
			_data.colormap = _defaultData.colormap;
			_commitRequired = true;
		}

		void ConstantBufferResource::CommitChangesToGpu(ID3D11DeviceContext* pImmediateContext)
		{
			if(_commitRequired)
				CopyToGpu(pImmediateContext);
		}



		HRESULT ConstantBufferResource::CopyToGpu(ID3D11DeviceContext* pImmediateContext)
		{
			HRESULT hr;

			D3D11_MAPPED_SUBRESOURCE mappedResource;
			ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

			// Disable GPU access 
			HR(pImmediateContext->Map(_pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

			// Copy data
			memcpy(mappedResource.pData, &_data, sizeof(CONSTANTBUFFERDATA));

			// Reenable GPU accces
			pImmediateContext->Unmap(_pBuffer, 0);

			_commitRequired = false;

		DXError:
			return hr;
		}

	} // namespace proc
} // namespace oct