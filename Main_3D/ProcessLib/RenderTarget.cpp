#include "stdafx.h"
#include "Utility.h"
#include "RenderTarget.h"
#include "Processor.h"


using namespace DirectX;
using namespace DirectX::PackedVector;


namespace oct {
	namespace proc {

		RenderTarget::RenderTarget(Processor* const pProcessor, Orientation orientation) :
			_pProcessor(pProcessor), _orientation(orientation)
		{
			assert(pProcessor);
			assert(typeid(pProcessor) == typeid(Processor*));

			// Verify that a source was loaded
			if (!pProcessor->HaveSource())
			{
				char msg[] = "Processor is not bound to a source. Can not create RenderTarget without a source.";
				OutputDebugStringA(msg);
				throw ProcessorException(msg);
			}

			// ViewPort
			_viewPort.MinDepth = 0.0f;
			_viewPort.MaxDepth = 1.0f;
						
			GpuProcessor *pGpuProcessor = _pProcessor->GetGpuProcessor();

			pGpuProcessor->AddToList(this);

		}


		RenderTarget::~RenderTarget()
		{
			SAFE_RELEASE(_pRenderTargetView);

			SAFE_DELETE(_pConstantBuffer);
			SAFE_DELETE(_pAxisXInfo);
			SAFE_DELETE(_pAxisYInfo);
		}

		double RenderTarget::GetNormalizedCatheterEdgePosition()
		{
			// Return normalized position of catheter edge
			// Position is relative to the start of the raw aline and normalized to its length
			
			OckFileSource* pSrc = _pProcessor->GetSource();
			AxisInfo alineAxis = pSrc->GetAxis1Info();
			double value = (pSrc->GetCatheterEdgePosition() - alineAxis.GetFirst()) / alineAxis.GetDelta();

			return value;
		}

		void RenderTarget::SetNormalizedCatheterEdgePosition(double value)
		{
			// Set then normalized position of catheter edge
			// Position is relative to the start of the raw aline and normalized to its length
			// The size of the catheter does not change. The offset is changed so the catheter edge appears to to move

			OckFileSource* pSrc = _pProcessor->GetSource();
			AxisInfo alineAxis = pSrc->GetAxis1Info();
			double catheterEdgePosition = value * alineAxis.GetDelta() + alineAxis.GetFirst();

			//pSrc->SetCatheterEdgePosition(catheterEdgePosition);

		}


		double RenderTarget::GetNormalizedOffsetPosition()
		{
			// Return normalized position of aline offset
			// Position is relative to the start of the raw aline and normalized to its length

			OckFileSource* pSrc = _pProcessor->GetSource();
			double offsetPosition = pSrc->GetALineOffset();
			AxisInfo alineAxis = pSrc->GetAxis1Info();
			double value = (offsetPosition - alineAxis.GetFirst()) / alineAxis.GetDelta();

			return value;
		}


		double RenderTarget::GetAspectRatio()
		{
			double aspectRatio = double(_fullScaleWidth) / double(_fullScaleHeight);
			return aspectRatio;
		}

		int RenderTarget::GetFullScaleWidth()
		{
			return _fullScaleWidth;
		}

		int RenderTarget::GetFullScaleHeight()
		{
			return _fullScaleHeight;
		}

		SIZE RenderTarget::GetScaledSize(double scale)
		{
			SIZE size;
			size.cx = (LONG)(scale *_fullScaleWidth);
			size.cy = (LONG)(scale *_fullScaleHeight);
			return size;
		}



		PixelInfo RenderTarget::GetPixelInfo(int x, int y)
		{
			x = CLAMP(x, 0, (int)_width - 1);
			y = CLAMP(y, 0, (int)_height - 1);
			
			PixelInfo pixelInfo = PixelInfo(x, y);

			pixelInfo.AcquisitionIndices = ConvertScreenCoordinatesToAcquistionIndices(x, y);
			pixelInfo.AcquisitionCoordinates = ConvertScreenCoordinatesToAcquisitionCoordinates(x, y);

			//int argb = GetPixelValueFromRenderTarget(x, y);
			PIXELINFO shaderPixelInfo = GetPixelInfoFromShader(x, y);
			pixelInfo.argb = shaderPixelInfo.brga;
			pixelInfo.value = shaderPixelInfo.value;


			return pixelInfo;
		}


		void RenderTarget::InitRenderTarget(void * pResource)
		{
			HRESULT hr;
			IUnknown *pUnk = (IUnknown*)pResource;
			GpuProcessor *pGpuProcessor = _pProcessor->GetGpuProcessor();
			ID3D11Device* const pd3dDevice = pGpuProcessor->GetDevice();
			
			// Get a handle to the DXGI resource (shared render target)
			HANDLE sharedHandle;
			IDXGIResource * pDXGIResource;
			HR(pUnk->QueryInterface(__uuidof(IDXGIResource), (void**)&pDXGIResource));
			HR(pDXGIResource->GetSharedHandle(&sharedHandle));

			// Get a pointer to the ID3D11Texture2D
			IUnknown * pSharedResource;
			ID3D11Texture2D * pSharedRenderTarget;
			HR(pd3dDevice->OpenSharedResource(sharedHandle, __uuidof(ID3D11Resource), (void**)(&pSharedResource)));
			HR(pSharedResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)(&pSharedRenderTarget)));

			// Releease the cuurent view target
			SAFE_RELEASE(_pRenderTargetView);
			
			// Create a new view target
			D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
			rtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtDesc.Texture2D.MipSlice = 0;
			HR(pd3dDevice->CreateRenderTargetView(pSharedRenderTarget, &rtDesc, &_pRenderTargetView));

			// Get the dimensions of the new render target
			D3D11_TEXTURE2D_DESC outputResourceDesc;
			pSharedRenderTarget->GetDesc(&outputResourceDesc);
			if (outputResourceDesc.Width != _width || outputResourceDesc.Height != _height)
			{
				// Size of render target has changed
				_width = outputResourceDesc.Width;
				_height = outputResourceDesc.Height;

				_viewPort.Width = float(outputResourceDesc.Width);
				_viewPort.Height = float(outputResourceDesc.Height);
			}

		DXError:

			// Clean up
			SAFE_RELEASE(pDXGIResource);
			SAFE_RELEASE(pSharedResource);
			SAFE_RELEASE(pSharedRenderTarget);

			if (FAILED(hr))
			{
				throw ProcessorException("Error initilizing render target.", hr);
			}
		}

		POINTD RenderTarget::ConvertScreenCoordinatesToAcquisitionCoordinates(int x, int y)
		{
			POINTD normalizedScreenCoordinates;
			normalizedScreenCoordinates.x = ScreenXToNormalizedScreenX(x);
			normalizedScreenCoordinates.y = ScreenYToNormalizedScreenY(y);

			POINTD p;
			p.x = _pAxisXInfo->GetFirst() + normalizedScreenCoordinates.x * _pAxisXInfo->GetDelta();
			p.y = _pAxisYInfo->GetFirst() + normalizedScreenCoordinates.y * _pAxisYInfo->GetDelta();

			return p;
		}

		int RenderTarget::GetPixelValueFromRenderTarget(int x, int y)
		{
			HRESULT hr;

			GpuProcessor* pGpuProcessor = _pProcessor->GetGpuProcessor();
			ID3D11DeviceContext* const pImmediateContext = pGpuProcessor->GetDeviceContext();

			ID3D11Texture2D *pRenderTarget;
			_pRenderTargetView->GetResource((ID3D11Resource**)(&pRenderTarget));

			D3D11_TEXTURE2D_DESC outputResourceDesc;
			pRenderTarget->GetDesc(&outputResourceDesc);

			outputResourceDesc.Height = 1;
			outputResourceDesc.Width = 1;
			outputResourceDesc.Usage = D3D11_USAGE_STAGING;
			outputResourceDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			outputResourceDesc.MiscFlags = 0;
			outputResourceDesc.BindFlags = 0;

			ID3D11Texture2D *pDstResource;
			pGpuProcessor->GetDevice()->CreateTexture2D(&outputResourceDesc, NULL, &pDstResource);

			// Copy pixel from GPU
			D3D11_BOX sourceRegion;
			sourceRegion.left = x;
			sourceRegion.right = x + 1;
			sourceRegion.top = y;
			sourceRegion.bottom = y + 1;
			sourceRegion.front = 0;
			sourceRegion.back = 1;
			pImmediateContext->CopySubresourceRegion(pDstResource, 0, 0, 0, 0, pRenderTarget, 0, &sourceRegion);


			// Read the value
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HR(pImmediateContext->Map(pDstResource, 0, D3D11_MAP_READ, 0, &mappedResource));
			int value = ((int*)mappedResource.pData)[0];
			pImmediateContext->Unmap(pDstResource, 0);

		DXError:

			SAFE_RELEASE(pRenderTarget);
			SAFE_RELEASE(pDstResource);



			return value;
		}


		PIXELINFO RenderTarget::GetPixelInfoFromShader(int x, int y)
		{
			HRESULT hr = S_OK;

			GpuProcessor* pGpuProcessor = _pProcessor->GetGpuProcessor();
			pGpuProcessor->BindInputResources(_pSrcData, _pConstantBuffer);
			return pGpuProcessor->RenderSinglePixel(x, y, &_viewPort, _orientation);
		}


		void RenderTarget::SetColormap(ColormapType type)
		{
			_pConstantBuffer->SetColormap(type);
			NotifyColormapChanged();
		}


		ColormapType RenderTarget::GetColormap()
		{
			return _pConstantBuffer->GetColormap();
		}

		ColormapType RenderTarget::GetColormapDefault()
		{
			return _pConstantBuffer->GetColormapDefault();
		}

		void  RenderTarget::SetWhiteLevel(float value)
		{
			_pConstantBuffer->SetWhiteLevel(value);
			NotifyLevelsChanged();
		}

		float  RenderTarget::GetWhiteLevel()
		{
			return _pConstantBuffer->GetWhiteLevel();
		}

		float  RenderTarget::GetWhiteLevelDefault()
		{
			return _pConstantBuffer->GetWhiteLevelDefault();
		}


		void  RenderTarget::SetBlackLevel(float value)
		{
			_pConstantBuffer->SetBlackLevel(value);
			NotifyLevelsChanged();
		}

		float  RenderTarget::GetBlackLevel()
		{
			return _pConstantBuffer->GetBlackLevel();
		}

		float  RenderTarget::GetBlackLevelDefault()
		{
			return _pConstantBuffer->GetBlackLevelDefault();
		}

		void RenderTarget::ResetLevels()
		{
			_pConstantBuffer->ResetLevels();
			NotifyLevelsChanged();
		}

		void RenderTarget::ResetColormap()
		{
			_pConstantBuffer->ResetColormap();
			NotifyColormapChanged();
		}

		void RenderTarget::NotifyColormapChanged()
		{
			// Notify host window. Window must rerender to show changes
			ColorChangedEventArgs args = ColorChangedEventArgs(this);
			OnColormapChanged(args);

			// Tell the processor the LUT has changed
			GpuProcessor* pGpuProcessor = _pProcessor->GetGpuProcessor();
			pGpuProcessor->NotifyColormapChanged(this);
		}

		void RenderTarget::NotifyLevelsChanged()
		{
			// Notify host window. Window must rerender to show changes
			ColorChangedEventArgs args = ColorChangedEventArgs(this);
			OnLevelsChanged(args);

			// Tell the processor the LUT has changed
			GpuProcessor* pGpuProcessor = _pProcessor->GetGpuProcessor();
			pGpuProcessor->NotifyLevelsChanged(this);
		}




	} // namespace proc
} // namespace oct