#include "stdafx.h"
#include "Utility.h"
#include "X1X2RenderTarget.h"
#include "Processor.h"


using namespace DirectX;
using namespace DirectX::PackedVector;



namespace oct {
	namespace proc {

		X1X2RenderTarget::X1X2RenderTarget(Processor* const pProcessor, Orientation orientation) :
			CartesianRenderTarget(pProcessor, orientation)
		{

			OckFileSource *pSource = pProcessor->GetSource();
			GpuProcessor* pGpuProcessor = _pProcessor->GetGpuProcessor();

			// Get default resources
			_imageModaility = ImageModailityType::OCT;
			_pSrcData = pGpuProcessor->GetX1X2Data();

			// Constant Buffer
			_pConstantBuffer = new ConstantBufferResource(pGpuProcessor->GetDevice(), -70.0f, 0.0f, ColormapType::Gray);

			// Set the aspect ratio in the X1-X2 coordinate system
			// X1 is the aline dimension
			// X2 is either linear or angular
			double radius, circumference, aspectRatio;
			AxisType axisType = pSource->GetAxis2Info().GetType();
			switch (axisType)
			{
			case AxisType::Angle:

				// Polar data
				radius = (pSource->GetAxis1Info().GetFirst() + pSource->GetAxis1Info().GetLast()) / 2.0;
				circumference = 2 * M_PI * radius;
				aspectRatio = pSource->GetAxis2Info().GetDelta() / circumference;
				break;

			case AxisType::Length:

				// cartesian data
				aspectRatio = pSource->GetAxis1Info().GetDelta() / pSource->GetAxis2Info().GetDelta();
				break;

			default:
				aspectRatio = 1.0;
			}

			// Set the fullscale width and height in the X1-X2 coordinate system
			UINT fullScaleWidth = UINT(_pSrcData->GetHeight() * aspectRatio);		// Resample the aline axis
			UINT fullScaleHeight = _pSrcData->GetHeight();							// no change

			// Transform into the local X-Y coordinate system
			if (_orientation & Orientation::Transpose)
			{
				_pAxisXInfo = new AxisInfo(pSource->GetAxis2Info());
				_pAxisYInfo = new AxisInfo(pSource->GetAxis1Info());
				_fullScaleWidth = fullScaleHeight;
				_fullScaleHeight = fullScaleWidth;
				_aquitionIndexRangeX[0] = 0;
				_aquitionIndexRangeX[1] = _pSrcData->GetHeight();
				_aquitionIndexRangeY[0] = 0;
				_aquitionIndexRangeY[1] = _pSrcData->GetWidth();
			}
			else
			{
				_pAxisXInfo = new AxisInfo(pSource->GetAxis1Info());
				_pAxisYInfo = new AxisInfo(pSource->GetAxis2Info());
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

		X1X2RenderTarget::~X1X2RenderTarget()
		{
			//SAFE_DELETE(_pPolarSection);
		}


		void X1X2RenderTarget::OnPositionChanged(ACQUISTIONPOINT oldPoint, ACQUISTIONPOINT newPoint, RenderTarget* pInitiator)
		{
			BOOL XChanged;
			BOOL YChanged;
			BOOL ContentChanged = (newPoint.X3 != oldPoint.X3);

			PositionChangedEventArgs args = PositionChangedEventArgs(oldPoint, newPoint);

			if (_orientation & Orientation::Transpose)
			{
				args.X = AcquitionIndexXToScreenX(newPoint.X2);
				args.Y = AcquitionIndexYToScreenY(newPoint.X1);
				XChanged = (newPoint.X2 != oldPoint.X2);
				YChanged = (newPoint.X1 != oldPoint.X1);
			}
			else
			{
				args.X = AcquitionIndexXToScreenX(newPoint.X1);
				args.Y = AcquitionIndexYToScreenY(newPoint.X2);
				XChanged = (newPoint.X1 != oldPoint.X1);
				YChanged = (newPoint.X2 != oldPoint.X2);
			}

			// Send events
			if (XChanged)
				OnCurrentPositionChangedX(args);

			if (YChanged)
				OnCurrentPositionChangedY(args);

			if (ContentChanged)
				OnContentChanged();
		}


		ACQUISTIONPOINT X1X2RenderTarget::ConvertScreenCoordinatesToAcquistionIndices(int x, int y)
		{
			ACQUISTIONPOINT p;
			p.X1 = ScreenXToAcquistionIndexX(x);
			p.X2 = ScreenYToAcquistionIndexY(y);
			p.X3 = _pProcessor->GetX3();

			if (_orientation & Orientation::Transpose)
				SWAP(p.X1, p.X2, int);

			return p;
		}

		void X1X2RenderTarget::SetCurrentPointX(int value)
		{
			int acquistionIndexX = ScreenXToAcquistionIndexX(value);
			if (_orientation & Orientation::Transpose)
			{
				_pProcessor->SetX2(acquistionIndexX, this);
			}
			else
			{
				_pProcessor->SetX1(acquistionIndexX, this);
			}
		}

		void X1X2RenderTarget::SetCurrentPointY(int value)
		{
			int acquistionIndexY = ScreenYToAcquistionIndexY(value);
			if (_orientation & Orientation::Transpose)
			{
				_pProcessor->SetX1(acquistionIndexY, this);
			}
			else
			{
				_pProcessor->SetX2(acquistionIndexY, this);
			}
		}

		POINT X1X2RenderTarget::GetCurrentPoint()
		{
			int acquistionIndexX = _pProcessor->GetX1();
			int acquistionIndexY = _pProcessor->GetX2();

			if (_orientation & Orientation::Transpose)
				SWAP(acquistionIndexX, acquistionIndexY, int);

			POINT p;
			p.x = NormalizedScreenXToScreenX(AcquitionIndexXToNormalizedScreenX(acquistionIndexX));
			p.y = NormalizedScreenYToScreenY(AcquitionIndexYToNormalizedScreenY(acquistionIndexY));

			return p;
		}

		void X1X2RenderTarget::Render(void * pResource, bool isNewSurface)
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

		CATHETERINFO X1X2RenderTarget::GetCatheterInfo()
		{
			double normPosition;
			int screenPosition;
			CatheterEdgeProjection projection;

			// Get position from source
			OckFileSource* pSrc = _pProcessor->GetSource();
			double diameter = pSrc->GetCatheterDiameter();				// mm
			double edgePosition = pSrc->GetCatheterEdgePosition();		// mm

			if (_orientation & Orientation::Transpose)
			{
				projection = CatheterEdgeProjection::HorizontalLine;	
				normPosition = (edgePosition - _pAxisYInfo->GetFirst()) / _pAxisYInfo->GetDelta();
				screenPosition = NormalizedScreenYToScreenY(normPosition);
			}
			else
			{
				projection = CatheterEdgeProjection::VerticalLine;
				normPosition = (edgePosition - _pAxisXInfo->GetFirst()) / _pAxisYInfo->GetDelta();
				screenPosition = NormalizedScreenXToScreenX(normPosition);
			}

			CATHETERINFO info = CATHETERINFO(diameter, projection);
			info.ScreenPosition = screenPosition;

			return info;
		}


		void X1X2RenderTarget::SetProposedCatheterEdgePosition(double normalizedPosition)
		{
			// Set value on processor
			// Processor will notify all render targets

			// Convert from normalized aline position to mm of air
			OckFileSource* pSrc = _pProcessor->GetSource();
			AxisInfo alineAxis = pSrc->GetAxis1Info();
			double position = normalizedPosition*alineAxis.GetDelta() + alineAxis.GetFirst();

			_pProcessor->SetProposedCatheterEdgePosition(position);
		}

		double X1X2RenderTarget::GetProposedCatheterEdgePosition()
		{
			double position = _pProcessor->GetProposedCatheterEdgePosition();

			// Convert from  mm of air to normalized aline position
			OckFileSource* pSrc = _pProcessor->GetSource();
			AxisInfo alineAxis = pSrc->GetAxis1Info();
			double normalizedPosition = (position - alineAxis.GetFirst()) / alineAxis.GetDelta();

			return normalizedPosition;
		}

		void X1X2RenderTarget::CommitProposedCatheterEdgePosition()
		{
			// TODO
		}

		void X1X2RenderTarget::NotifyCatheterEdgePositionChanged(double position, RenderTarget* pInitiator)
		{
			// Notify container hosting the render target that the catheter needs to be redrawn at a new position

			// Convert from  mm of air to normalized aline position
			OckFileSource* pSrc = _pProcessor->GetSource();
			AxisInfo alineAxis = pSrc->GetAxis1Info();
			double normalizedPosition = (position - alineAxis.GetFirst()) / alineAxis.GetDelta();

			PositionChangedEventArgs args;
			args.Rho = normalizedPosition;
			OnCatheterEdgePositionChanged(args);
		}

	} // namespace proc
} // namespace oct