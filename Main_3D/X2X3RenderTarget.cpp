#include "stdafx.h"
#include "Utility.h"
#include "X2X3RenderTarget.h"
#include "Processor.h"


using namespace DirectX;
using namespace DirectX::PackedVector;



namespace oct {
	namespace proc {

		X2X3RenderTarget::X2X3RenderTarget(Processor* const pProcessor, Orientation orientation) :
			CartesianRenderTarget(pProcessor, orientation)
		{

			OckFileSource *pSource = pProcessor->GetSource();
			GpuProcessor* pGpuProcessor = _pProcessor->GetGpuProcessor();

			// Get default resources
			_imageModaility = ImageModailityType::AFI;
			_pSrcData = pGpuProcessor->GetAfiData();

			// Constant Buffer
			_pConstantBuffer = new ConstantBufferResource(pGpuProcessor->GetDevice(), 2496 / 32767.0f, 13113 / 32767.0f, ColormapType::Green);


			// Set the aspect ratio in the X2-X3 coordinate system
			double aspectRatio = pSource->GetAxis2Info().GetDelta() / pSource->GetAxis3Info().GetDelta();

			// Set the fullscale width and height in the X1-X2 coordinate system
			UINT fullScaleWidth = UINT(_pSrcData->GetHeight() * aspectRatio);		// Resample the aline axis
			UINT fullScaleHeight = _pSrcData->GetHeight();							// no change

			// Transform into the local X-Y coordinate system
			if (_orientation & Orientation::Transpose)
			{
				_pAxisXInfo = new AxisInfo(pSource->GetAxis3Info());
				_pAxisYInfo = new AxisInfo(pSource->GetAxis2Info());
				_fullScaleWidth = fullScaleHeight;
				_fullScaleHeight = fullScaleWidth;
				_aquitionIndexRangeX[0] = 0;
				_aquitionIndexRangeX[1] = _pSrcData->GetHeight();
				_aquitionIndexRangeY[0] = 0;
				_aquitionIndexRangeY[1] = _pSrcData->GetWidth();
			}
			else
			{
				_pAxisXInfo = new AxisInfo(pSource->GetAxis2Info());
				_pAxisYInfo = new AxisInfo(pSource->GetAxis3Info());
				_fullScaleWidth = fullScaleWidth;
				_fullScaleHeight = fullScaleHeight;
				_aquitionIndexRangeX[0] = 0;
				_aquitionIndexRangeX[1] = _pSrcData->GetWidth();
				_aquitionIndexRangeY[0] = 0;
				_aquitionIndexRangeY[1] = _pSrcData->GetHeight();
			}
			if (_orientation & Orientation::FlipLeftRight)
			{
				_pAxisXInfo->SwapFirstLast();
				SWAP(_aquitionIndexRangeX[0], _aquitionIndexRangeX[1], int);
			}
			if (_orientation & Orientation::FlipTopBottom)
			{
				_pAxisYInfo->SwapFirstLast();
				SWAP(_aquitionIndexRangeY[0], _aquitionIndexRangeY[1], int);
			}

		}

		X2X3RenderTarget::~X2X3RenderTarget()
		{

		}


		void X2X3RenderTarget::OnPositionChanged(ACQUISTIONPOINT oldPoint, ACQUISTIONPOINT newPoint, RenderTarget* pInitiator)
		{
			BOOL XChanged;
			BOOL YChanged;
			BOOL ContentChanged = (newPoint.X1 != oldPoint.X1);

			PositionChangedEventArgs args = PositionChangedEventArgs(oldPoint, newPoint);
			
			if (_orientation & Orientation::Transpose)
			{
				args.X = AcquitionIndexXToScreenX(newPoint.X3);
				args.Y = AcquitionIndexYToScreenY(newPoint.X2);
				XChanged = (newPoint.X3 != oldPoint.X3);
				YChanged = (newPoint.X2 != oldPoint.X2);
			}
			else
			{
				args.X = AcquitionIndexXToScreenX(newPoint.X2);
				args.Y = AcquitionIndexYToScreenY(newPoint.X3);
				XChanged = (newPoint.X2 != oldPoint.X2);
				YChanged = (newPoint.X3 != oldPoint.X3);
			}

			// Send events
			if (XChanged)
				OnCurrentPositionChangedX(args);

			if (YChanged)
				OnCurrentPositionChangedY(args);

			if (ContentChanged)
				OnContentChanged();
		}


		ACQUISTIONPOINT X2X3RenderTarget::ConvertScreenCoordinatesToAcquistionIndices(int x, int y)
		{
			ACQUISTIONPOINT p;
			p.X1 = _pProcessor->GetX1();
			p.X2 = ScreenXToAcquistionIndexX(x);
			p.X3 = ScreenYToAcquistionIndexY(y);

			if (_orientation & Orientation::Transpose)
				SWAP(p.X2, p.X3, int);

			return p;
		}

		void X2X3RenderTarget::SetCurrentPointX(int value)
		{
			int acquistionIndexX = ScreenXToAcquistionIndexX(value);
			if (_orientation & Orientation::Transpose)
			{
				_pProcessor->SetX3(acquistionIndexX, this);
			}
			else
			{
				_pProcessor->SetX2(acquistionIndexX, this);
			}
		}

		void X2X3RenderTarget::SetCurrentPointY(int value)
		{
			int acquistionIndexY = ScreenYToAcquistionIndexY(value);
			if (_orientation & Orientation::Transpose)
			{
				_pProcessor->SetX2(acquistionIndexY, this);
			}
			else
			{
				_pProcessor->SetX3(acquistionIndexY, this);
			}
		}


		POINT X2X3RenderTarget::GetCurrentPoint()
		{
			int acquistionIndexX = _pProcessor->GetX2();
			int acquistionIndexY = _pProcessor->GetX3();

			if (_orientation & Orientation::Transpose)
				SWAP(acquistionIndexX, acquistionIndexY, int);

			POINT p;
			p.x = NormalizedScreenXToScreenX(AcquitionIndexXToNormalizedScreenX(acquistionIndexX));
			p.y = NormalizedScreenYToScreenY(AcquitionIndexYToNormalizedScreenY(acquistionIndexY));

			return p;
		}


		void X2X3RenderTarget::Render(void * pResource, bool isNewSurface)
		{
			HRESULT hr = S_OK;

			GpuProcessor* pGpuProcessor = _pProcessor->GetGpuProcessor();

			// If we've gotten a new Surface, need to initialize the renderTarget.
			// One of the times that this happens is on a resize.
			if (isNewSurface)
			{
				InitRenderTarget(pResource);
			}
			
			pGpuProcessor->BindInputResources(_pSrcData, _pConstantBuffer);
			pGpuProcessor->Render(_pRenderTargetView, &_viewPort, _orientation);
			
		}

		// CATHETER POSITION

		CATHETERINFO X2X3RenderTarget::GetCatheterInfo()
		{
			// Get position from source
			OckFileSource* pSrc = _pProcessor->GetSource();
			double diameter = pSrc->GetCatheterDiameter();				// mm

			return CATHETERINFO(diameter, CatheterEdgeProjection::Hidden);
		}


		void X2X3RenderTarget::SetProposedCatheterEdgePosition(double normalizedPosition)
		{
			//// Set value on processor
			//// Processor will notify all render targets

			//// Convert from normalized aline position to mm of air
			//OckFileSource* pSrc = _pProcessor->GetSource();
			//AxisInfo alineAxis = pSrc->GetAxis1Info();
			//double position = normalizedPosition*alineAxis.GetDelta() + alineAxis.GetFirst();

			//_pProcessor->SetProposedCatheterEdgePosition(position);
		}

		double X2X3RenderTarget::GetProposedCatheterEdgePosition()
		{
			//double position = _pProcessor->GetProposedCatheterEdgePosition();

			//// Convert from  mm of air to normalized aline position
			//OckFileSource* pSrc = _pProcessor->GetSource();
			//AxisInfo alineAxis = pSrc->GetAxis1Info();
			//double normalizedPosition = (position - alineAxis.GetFirst()) / alineAxis.GetDelta();

			return 0;
		}

		void X2X3RenderTarget::CommitProposedCatheterEdgePosition()
		{
			// TODO
		}

		void X2X3RenderTarget::NotifyCatheterEdgePositionChanged(double position, RenderTarget* pInitiator)
		{
			//// Notify container hosting the render target that the catheter needs to be redrawn at a new position

			//// Convert from  mm of air to normalized aline position
			//OckFileSource* pSrc = _pProcessor->GetSource();
			//AxisInfo alineAxis = pSrc->GetAxis1Info();
			//double normalizedPosition = (position - alineAxis.GetFirst()) / alineAxis.GetDelta();

			//PositionChangedEventArgs args;
			//args.Rho = normalizedPosition;
			//OnCatheterEdgePositionChanged(args);
		}

		
	} // namespace proc
} // namespace oct