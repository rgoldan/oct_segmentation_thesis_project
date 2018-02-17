#include "stdafx.h"
#include "GpuProcessor.h"

#include "VertexShader.h"
#include "PixelShader.h"
#include "PolarPixelShader.h"
#include "PixelInfoShader.h"

using namespace DirectX;
using namespace DirectX::PackedVector;


struct SimpleVertex
{
	XMFLOAT2 Pos;
	XMFLOAT2 Tex;
};

namespace oct {
	namespace proc {

		GpuProcessor::GpuProcessor()
		{
			InitDevice();
		}


		GpuProcessor::~GpuProcessor()
		{
			CleanUp();
		}


		void GpuProcessor::InitDevice()
		{
			HRESULT hr;
						
			// Create resources

			HR(CreateDevice(&_pd3dDevice, &_pImmediateContext, &_pDebug));
			
			HR(_pd3dDevice->CreatePixelShader(g_PixelShader, sizeof(g_PixelShader), NULL, &_pPixelShader));
			HR(_pd3dDevice->CreatePixelShader(g_PixelInfoShader, sizeof(g_PixelInfoShader), NULL, &_pPixelInfoShader));

			HR(_pd3dDevice->CreatePixelShader(g_PolarPixelShader, sizeof(g_PolarPixelShader), NULL, &_pPolarPixelShader));

			HR(CreateRasterizerStateResource(_pd3dDevice, &_pDefaultRasterizerState, false));
			HR(CreateRasterizerStateResource(_pd3dDevice, &_pScissorEnabledRasterizerState, true));

			HR(CreateGeometryResources(_pd3dDevice, &_pVertexBuffer, &_pIndexBuffer, &_pVertexLayout, &_pVertexShader));
			HR(CreateSamplerResource(_pd3dDevice, &_pDataSampler, D3D11_FILTER_ANISOTROPIC, 8));
			HR(CreateSamplerResource(_pd3dDevice, &_pColormapSampler, D3D11_FILTER_MIN_MAG_MIP_LINEAR, 0));
			

			_pColormapArray = new ColormapArrayResource(_pd3dDevice);

			_pPixelInfo = new PixelInfoResource(_pd3dDevice);

			// Setup geometry
			UINT offset = 0;
			UINT stride = sizeof(SimpleVertex);
			_pImmediateContext->IASetInputLayout(_pVertexLayout);
			_pImmediateContext->IASetVertexBuffers(0, 1, &_pVertexBuffer, &stride, &offset);
			_pImmediateContext->IASetIndexBuffer(_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
			_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// Vertex shader
			_pImmediateContext->VSSetShader(_pVertexShader, NULL, 0);

			// Bind Samplers
			ID3D11SamplerState *state[] = { _pColormapSampler, _pDataSampler  };
			_pImmediateContext->PSSetSamplers(0, 2, state);

			// Bind colormaps
			ID3D11ShaderResourceView *srv[] = { _pColormapArray->GetLutSRV() };
			_pImmediateContext->PSSetShaderResources(0, 1, srv);
			
					
		DXError:
			if (FAILED(hr))
			{
				CleanUp();
				char msg[256];
				GetErrorMessage(hr, msg, sizeof(msg));
				throw ProcessorException(msg, hr);
			}

		}

		void GpuProcessor::InitDataResources(int X1Length, int X2Length, int X3Length)
		{
			_pX1X2Data = new DataResource2D(_pd3dDevice, X1Length, X2Length, DXGI_FORMAT_R32_FLOAT);
			_pAfiData = new DataResource2D(_pd3dDevice, X2Length, X3Length, DXGI_FORMAT_R16_SNORM);

		}



		void GpuProcessor::CleanUp()
		{
			if (_pImmediateContext)
			{
				_pImmediateContext->ClearState();
			}


			SAFE_DELETE(_pX1X2Data);
			SAFE_DELETE(_pAfiData);
			SAFE_DELETE(_pColormapArray);
			SAFE_DELETE(_pPixelInfo);

			SAFE_RELEASE(_pIndexBuffer);
			SAFE_RELEASE(_pPixelShader);
			SAFE_RELEASE(_pPolarPixelShader);
			SAFE_RELEASE(_pPixelInfoShader);
			SAFE_RELEASE(_pVertexBuffer);
			SAFE_RELEASE(_pVertexLayout);
			SAFE_RELEASE(_pVertexShader);
			SAFE_RELEASE(_pImmediateContext);
			SAFE_RELEASE(_pDataSampler);
			SAFE_RELEASE(_pColormapSampler);

			SAFE_RELEASE(_pDefaultRasterizerState);
			SAFE_RELEASE(_pScissorEnabledRasterizerState);

			// Uncomment help find memory leaks
			// All objeects should have Refcount=0 except ID3D11Device
			//m_pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);


			SAFE_RELEASE(_pDebug);
			SAFE_RELEASE(_pd3dDevice);
		}



		void GpuProcessor::AddToList(RenderTarget* const pRenderTarget)
		{
			_renderTargets.push_back(pRenderTarget);
		}

		void GpuProcessor::NotifyPositionChanged(ACQUISTIONPOINT oldPoint, ACQUISTIONPOINT newPoint, RenderTarget* pInitiator)
		{

			// Send position changed event to all
			for (std::vector<RenderTarget*>::iterator it = _renderTargets.begin(); it != _renderTargets.end(); ++it)
			{
				RenderTarget* pRenderTarget = *it;
				pRenderTarget->OnPositionChanged(oldPoint, newPoint, pInitiator);
			}
		}

		void GpuProcessor::NotifyCatheterEdgePositionChanged(double position, RenderTarget* pInitiator)
		{

			// Send notification to all
			for (std::vector<RenderTarget*>::iterator it = _renderTargets.begin(); it != _renderTargets.end(); ++it)
			{
				RenderTarget* pRenderTarget = *it;
				pRenderTarget->NotifyCatheterEdgePositionChanged(position, pInitiator);
			}
		}


		void GpuProcessor::NotifyLevelsChanged(RenderTarget* pInitiator)
		{
			BOOL LinkLevels = true;

			float black = pInitiator->GetBlackLevel();
			float white = pInitiator->GetWhiteLevel();

			// Send position changed event to all
			for (std::vector<RenderTarget*>::iterator it = _renderTargets.begin(); it != _renderTargets.end(); ++it)
			{
				RenderTarget* pRenderTarget = *it;
								
				// Don't change render target that inititated the change
				if (pRenderTarget == pInitiator) continue;
								
				// Only change the levels if the image modalities match
				if (pRenderTarget->GetImageModality() == pInitiator->GetImageModality())
				{
					// Update the levels
					ConstantBufferResource* pConstantBuffer = pRenderTarget->GetConstantBuffer();
					pConstantBuffer->SetBlackLevel(black);
					pConstantBuffer->SetWhiteLevel(white);

					// Notify 
					ColorChangedEventArgs args = ColorChangedEventArgs(pRenderTarget);
					pRenderTarget->OnLevelsChanged(args);
				}
				
			}
		}

		void GpuProcessor::NotifyColormapChanged(RenderTarget* pInitiator)
		{
			BOOL LinkColormaps = true;

			ColormapType colormap = pInitiator->GetColormap();

			// Send position changed event to all
			for (std::vector<RenderTarget*>::iterator it = _renderTargets.begin(); it != _renderTargets.end(); ++it)
			{
				RenderTarget* pRenderTarget = *it;

				// Don't change render target that inititated the change
				if (pRenderTarget == pInitiator) continue;

				// Only change the colormap if the image modalities match
				if (pRenderTarget->GetImageModality() == pInitiator->GetImageModality())
				{
					// Update the colormap
					ConstantBufferResource* pConstantBuffer = pRenderTarget->GetConstantBuffer();
					pConstantBuffer->SetColormap(colormap);

					// Notify 
					ColorChangedEventArgs args = ColorChangedEventArgs(pRenderTarget);
					pRenderTarget->OnLevelsChanged(args);
				}

			}
		}


		void GpuProcessor::CopyToX2X1Section(const void *pData, int rowPitch)
		{
			// Copy the new content
			_pX1X2Data->CopyToGpu(_pImmediateContext, pData, rowPitch);
		}



		void GpuProcessor::SetAfiData(OckFileSource *pSource)
		{
			int width = pSource->GetAxis2Length();
			int height = pSource->GetAxis3Length();

			// Build AFI image
			int stepBytes;
			Ipp16s *buf = ippiMalloc_16s_C1(width, height, &stepBytes);
			pSource->CopyFluorescenceDataToBuffer(buf, stepBytes, width, height);

			//// Gamma Check
			//int X1 = width / 2;
			//	
			//for (int x = 0; x < X1; x++)
			//{
			//	buf[x] = (x % 2) ? 255 : 0;
			//}
			//for (int x = X1; x < width; x++)
			//{
			//	buf[x] = 128;
			//}
			//Ipp16s *pDst = buf + (stepBytes >> 1);
			//for (int y = 1; y < height; y++)
			//{
			//	ippsCopy_16s(buf, pDst, width);
			//	pDst += stepBytes >> 1;
			//}


			_pAfiData->CopyToGpu(_pImmediateContext, buf, stepBytes);

			ippiFree(buf);

		}

		void GpuProcessor::BindInputResources(DataResource2D *pData, ConstantBufferResource *pConstantBuffer)
		{
			
			// Constant buffers
			if (pConstantBuffer->IsCommitToGpuRequired())
				pConstantBuffer->CommitChangesToGpu(_pImmediateContext);
			ID3D11Buffer *constantBuffers[] = { pConstantBuffer->GetBuffer() };
			_pImmediateContext->PSSetConstantBuffers(0, 1, constantBuffers);
			
			// Shader resources
			ID3D11ShaderResourceView *srv[] = { pData->GetShaderResourceView() };
			_pImmediateContext->PSSetShaderResources(1, 1, srv);
		}


		void GpuProcessor::Render(ID3D11RenderTargetView *pView, D3D11_VIEWPORT *pViewPort, Orientation orientation)
		{
			// Bind render target
			ID3D11RenderTargetView *rtv[] = { pView };
			_pImmediateContext->OMSetRenderTargets(1, rtv, NULL);

			// Set the pixel shader
			_pImmediateContext->PSSetShader(_pPixelShader, NULL, 0);
			
			_pImmediateContext->RSSetState(_pDefaultRasterizerState);

			_pImmediateContext->RSSetViewports(1, pViewPort);

			// Draw 6 vertices
			int index = 6 * static_cast<int>(orientation);
			_pImmediateContext->DrawIndexed(6, index, 0);
			
			_pImmediateContext->Flush();
		}


		void GpuProcessor::RenderPolar(ID3D11RenderTargetView *pView, D3D11_VIEWPORT *pViewPort, Orientation orientation)
		{
			// Bind render target
			ID3D11RenderTargetView *rtv[] = { pView };
			_pImmediateContext->OMSetRenderTargets(1, rtv, NULL);

			// Set the pixel shader
			_pImmediateContext->PSSetShader(_pPolarPixelShader, NULL, 0);

			_pImmediateContext->RSSetState(_pDefaultRasterizerState);

			_pImmediateContext->RSSetViewports(1, pViewPort);

			// Draw 6 vertices
			int index = 6 * static_cast<int>(orientation);
			_pImmediateContext->DrawIndexed(6, index, 0);

			_pImmediateContext->Flush();
		}


		PIXELINFO GpuProcessor::RenderSinglePixel(int x, int y, D3D11_VIEWPORT *pViewPort, Orientation orientation)
		{
			// Set the pixel shader
			_pImmediateContext->PSSetShader(_pPixelInfoShader, NULL, 0);
			
			// Scissor Box defines pixel to querry
			D3D11_RECT rect;
			rect.left = x;
			rect.right = x + 1;
			rect.top = y;
			rect.bottom = y + 1;
			_pImmediateContext->RSSetScissorRects(1, &rect);
			_pImmediateContext->RSSetState(_pScissorEnabledRasterizerState);
			_pImmediateContext->RSSetViewports(1, pViewPort);

			// Bind UAV
			ID3D11UnorderedAccessView *uav[] = { _pPixelInfo->GetUAV() };
			_pImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, 1, 1, uav, NULL);

			// Draw 6 vertices
			int index = 6 * static_cast<int>(orientation);
			_pImmediateContext->DrawIndexed(6, index, 0);
			_pImmediateContext->Flush();

			// Get the result
			return _pPixelInfo->GetPixelInfo(_pImmediateContext);
		}


		////////////////////////////////////////
		//HELPERS


		HRESULT CreateDevice(ID3D11Device **ppDevice, ID3D11DeviceContext **ppImmediateContext, ID3D11Debug **ppDebugInterface)
		{
			HRESULT hr;

			UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(DEBUG) || defined(_DEBUG)
			createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

			// DX10 or 11 devices are suitable
			D3D_FEATURE_LEVEL featureLevels[] =
			{
				D3D_FEATURE_LEVEL_11_0,
				//D3D_FEATURE_LEVEL_10_1,
				//D3D_FEATURE_LEVEL_10_0,
			};
			UINT numFeatureLevels = ARRAYSIZE(featureLevels);


			HR(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
					D3D11_SDK_VERSION, ppDevice, NULL, ppImmediateContext));

			// Dubug interface
#if defined(DEBUG) || defined(_DEBUG)
			HR((*ppDevice)->QueryInterface(IID_PPV_ARGS(ppDebugInterface)));
#endif

		DXError:
			if (FAILED(hr))
			{
				SAFE_RELEASE(*ppDevice);
				SAFE_RELEASE(*ppImmediateContext);
				SAFE_RELEASE(*ppDevice);
			}

			return hr;
		}


		HRESULT CreateGeometryResources(ID3D11Device *pDevice, ID3D11Buffer **ppVertexBuffer, ID3D11Buffer **ppIndexBuffer, ID3D11InputLayout **ppVertexLayout, ID3D11VertexShader **ppVertexShader)
		{
			HRESULT hr;

			// Vertex space (x,y)
			const XMFLOAT2 vertTopLeft =		XMFLOAT2(-1.0f, 1.0f);
			const XMFLOAT2 vertTopRight =		XMFLOAT2(1.0f, 1.0f);
			const XMFLOAT2 vertBottomLeft =		XMFLOAT2(-1.0f, -1.0f);
			const XMFLOAT2 vertBottomRight =	XMFLOAT2(1.0f, -1.0f);

			// Texture space (u,v)
			const XMFLOAT2 texTopLeft =			XMFLOAT2(0.0f, 0.0f);
			const XMFLOAT2 texTopRight =		XMFLOAT2(1.0f, 0.0f);
			const XMFLOAT2 texBottomLeft =		XMFLOAT2(0.0f, 1.0f);
			const XMFLOAT2 texBottomRight =		XMFLOAT2(1.0f, 1.0f);

			// Define the geometry
			const SimpleVertex vertices[] =
			{
				// Default (0)
				{ vertBottomLeft,	texBottomLeft	},
				{ vertTopLeft,		texTopLeft		},
				{ vertTopRight,		texTopRight		},
				{ vertBottomRight,	texBottomRight	},

				// FlipLeftRight (1)
				{ vertBottomLeft,	texBottomRight	},
				{ vertTopLeft,		texTopRight		},
				{ vertTopRight,		texTopLeft		},
				{ vertBottomRight,	texBottomLeft	},

				// FlipTopBottom (2)
				{ vertBottomLeft,	texTopLeft		},
				{ vertTopLeft,		texBottomLeft	},
				{ vertTopRight,		texBottomRight	},
				{ vertBottomRight,	texTopRight		},

				// FlipLeftRight | FlipTopBottom (3)
				{ vertBottomLeft,	texTopRight		},
				{ vertTopLeft,		texBottomRight	},
				{ vertTopRight,		texBottomLeft	},
				{ vertBottomRight,	texTopLeft		},

				// Transpose (4)
				{ vertBottomLeft,	texTopRight		},
				{ vertTopLeft,		texTopLeft		},	
				{ vertTopRight,		texBottomLeft	},
				{ vertBottomRight,	texBottomRight	},

				// Transpose | FlipLeftRight (5)
				{ vertBottomLeft,	texBottomRight	},
				{ vertTopLeft,		texBottomLeft	},
				{ vertTopRight,		texTopLeft		},
				{ vertBottomRight,	texTopRight		},

				// Transpose | FlipTopBottom (6)
				{ vertBottomLeft,	texTopLeft		},
				{ vertTopLeft,		texTopRight		},
				{ vertTopRight,		texBottomRight	},
				{ vertBottomRight,	texBottomLeft	},

				// Transpose FlipLeftRight | FlipTopBottom (7)
				{ vertBottomLeft,	texBottomLeft	},
				{ vertTopLeft,		texBottomRight  },
				{ vertTopRight,		texTopRight		},
				{ vertBottomRight,	texTopLeft		},

			};
			const WORD indices[] =
			{
				// NoTransposeNoFlip
				0, 1, 3,
				1, 2, 3,

				// NoTransposeFlipLeftRight
				4, 5, 7,
				5, 6, 7,

				// NoTransposeFlipUpDown
				8, 9, 11,
				9, 10, 11,
				
				// NoTransposeFlipBoth
				12, 13, 15,
				13, 14, 15,

				// TransposeNoFlip
				16, 17, 19,
				17, 18, 19,

				// TransposeFlipLeftRight
				20, 21, 23,
				21, 22, 23,

				// TransposeFlipUpDown
				24, 25, 27,
				25, 26, 27,

				// TransposeFlipBoth
				28, 29, 31,
				29, 30, 31,

			};

			// Define the input layout
			const D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			// Vertex buffer
			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.ByteWidth = sizeof(vertices);
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA initData;
			ZeroMemory(&initData, sizeof(initData));
			initData.pSysMem = vertices;
			HR(pDevice->CreateBuffer(&desc, &initData, ppVertexBuffer));

			// Index buffer
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.ByteWidth = sizeof(indices);
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			initData.pSysMem = indices;
			HR(pDevice->CreateBuffer(&desc, &initData, ppIndexBuffer));

			// Vertex shader
			HR(pDevice->CreateVertexShader(g_VertexShader, sizeof(g_VertexShader), NULL, ppVertexShader));

			// Input layout
			HR(pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), g_VertexShader, sizeof(g_VertexShader), ppVertexLayout));

		DXError:
			if (FAILED(hr))
			{
				SAFE_RELEASE(*ppVertexBuffer);
				SAFE_RELEASE(*ppIndexBuffer);
				SAFE_RELEASE(*ppVertexLayout);
				SAFE_RELEASE(*ppVertexShader);
			}

			return hr;
		}


		HRESULT CreateSamplerResource(ID3D11Device *pDevice, ID3D11SamplerState **ppSamplerState, D3D11_FILTER filter, UINT maxAnisotropy)
		{
			HRESULT hr;

			// Sampler
			D3D11_SAMPLER_DESC desc;
			ZeroMemory(&desc, sizeof(D3D11_SAMPLER_DESC));
			desc.Filter = filter;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.MaxAnisotropy = maxAnisotropy;
			desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

			HR(pDevice->CreateSamplerState(&desc, ppSamplerState));

		DXError:
			if (FAILED(hr))
			{
				SAFE_RELEASE(*ppSamplerState);
			}

			return hr;
		}


		HRESULT CreateRasterizerStateResource(ID3D11Device *pDevice, ID3D11RasterizerState **ppState, BOOL scissorEnable)
		{

			HRESULT hr;

			D3D11_RASTERIZER_DESC desc;
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_BACK;
			desc.FrontCounterClockwise = false;
			desc.DepthBias = 0;
			desc.DepthBiasClamp = 0.0f;
			desc.SlopeScaledDepthBias = 0.0f;
			desc.DepthClipEnable = true;
			desc.ScissorEnable = scissorEnable;
			desc.MultisampleEnable = false;
			desc.AntialiasedLineEnable = false;
			HR(pDevice->CreateRasterizerState(&desc, ppState));
		
		DXError:
			if (FAILED(hr))
			{
				SAFE_RELEASE(*ppState);
			}

			return hr;
		}




	} // namespace proc
} // namespace oct