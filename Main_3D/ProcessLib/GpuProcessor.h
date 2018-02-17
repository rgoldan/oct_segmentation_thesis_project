#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include <vector>

#include "Utility.h"
#include "OckFileSource.h"
#include "DataResource2D.h"
#include "ConstantBufferResource.h"
#include "ColormapArrayResource.h"

#include "X1X2RenderTarget.h"
#include "X2X3RenderTarget.h"
#include "R1T2RenderTarget.h"



namespace oct {
	namespace proc {

		// Helpers
		HRESULT CreateDevice(ID3D11Device **ppDevice, ID3D11DeviceContext **ppImmediateContext, ID3D11Debug **ppDebugInterface);
		HRESULT CreateGeometryResources(ID3D11Device *pDevice, ID3D11Buffer **ppVertexBuffer, ID3D11Buffer **ppIndexBuffer, ID3D11InputLayout **ppVertexLayout, ID3D11VertexShader **ppVertexShader);
		HRESULT CreateSamplerResource(ID3D11Device *pDevice, ID3D11SamplerState **ppSamplerState, D3D11_FILTER filter, UINT maxAnisotropy);
		HRESULT CreateRasterizerStateResource(ID3D11Device *pDevice, ID3D11RasterizerState **ppState, BOOL scissorEnable);

			
		class GpuProcessor
		{

		public:

			GpuProcessor();
			~GpuProcessor();

			void AddToList(RenderTarget* const pRenderTarget);
			void NotifyPositionChanged(ACQUISTIONPOINT oldPoint, ACQUISTIONPOINT newPoint, RenderTarget* pInitiator);
			void NotifyCatheterEdgePositionChanged(double value, RenderTarget* pInitiator);
			void NotifyLevelsChanged(RenderTarget* pInitiator);
			void NotifyColormapChanged(RenderTarget* pInitiator);

			void InitDataResources(int X1Length, int X2Length, int X3Length);
			void BindInputResources(DataResource2D *pData, ConstantBufferResource *pConstantBuffer);
			
			void Render(ID3D11RenderTargetView *pView, D3D11_VIEWPORT *pViewPort, Orientation orientation);
			void RenderPolar(ID3D11RenderTargetView *pView, D3D11_VIEWPORT *pViewPort, Orientation orientation);

			PIXELINFO RenderSinglePixel(int x, int y, D3D11_VIEWPORT *pViewPort, Orientation orientation);
			

			ID3D11Device* const GetDevice()
			{
				return _pd3dDevice;
			}
			ID3D11DeviceContext* const GetDeviceContext()
			{
				return _pImmediateContext;
			}

			void CopyToX2X1Section(const void *pData, int rowPitch);
			void SetAfiData(OckFileSource *pSource);

			UINT GetLengthX1()
			{
				return _pX1X2Data->GetWidth();
			}
			UINT GetLengthX2()
			{
				return _pX1X2Data->GetHeight();
			}

			DataResource2D* GetX1X2Data()
			{
				return _pX1X2Data;
			}

			DataResource2D* GetAfiData()
			{
				return _pAfiData;
			}

		private:

			void InitDevice();
			void CleanUp();

			std::vector<RenderTarget*>			_renderTargets;
			
			ID3D11Device*                       _pd3dDevice = NULL;
			ID3D11DeviceContext*                _pImmediateContext = NULL;
			ID3D11InputLayout*                  _pVertexLayout = NULL;
			ID3D11Buffer*                       _pVertexBuffer = NULL;
			ID3D11Buffer*                       _pIndexBuffer = NULL;

			ID3D11Debug*						_pDebug = NULL;

			ID3D11VertexShader*                 _pVertexShader = NULL;
			ID3D11PixelShader*                  _pPixelShader = NULL;
			ID3D11PixelShader*                  _pPixelInfoShader = NULL;

			ID3D11PixelShader*                  _pPolarPixelShader = NULL;

			DataResource2D*						_pX1X2Data = NULL;
			DataResource2D*						_pAfiData = NULL;

			ColormapArrayResource*				_pColormapArray = NULL;
			
			PixelInfoResource*					_pPixelInfo = NULL;

			ID3D11SamplerState*					_pDataSampler = NULL;
			ID3D11SamplerState*					_pColormapSampler = NULL;

			ID3D11RasterizerState*				_pDefaultRasterizerState = NULL;
			ID3D11RasterizerState*				_pScissorEnabledRasterizerState = NULL;

		};

	} // namespace proc
} // namespace oct