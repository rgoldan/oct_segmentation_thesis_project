#include "stdafx.h"
#include "Utility.h"
#include "R1T2RenderTarget.h"
#include "Processor.h"
#include "math.h"


using namespace DirectX;
using namespace DirectX::PackedVector;



namespace oct {
	namespace proc {

		R1T2RenderTarget::R1T2RenderTarget(Processor* const pProcessor, Orientation orientation) :
			PolarRenderTarget(pProcessor, orientation)
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
			//double radius, circumference, aspectRatio;
			//AxisType axisType = pSource->GetAxis2Info().GetType();
			//switch (axisType)
			//{
			//case AxisType::Angle:

			//	// Polar data
			//	radius = (pSource->GetAxis1Info().GetFirst() + pSource->GetAxis1Info().GetLast()) / 2.0;
			//	circumference = 2 * M_PI * radius;
			//	aspectRatio = pSource->GetAxis2Info().GetDelta() / circumference;
			//	break;

			//case AxisType::Length:

			//	// cartesian data
			//	aspectRatio = pSource->GetAxis1Info().GetDelta() / pSource->GetAxis2Info().GetDelta();
			//	break;

			//default:
			//	aspectRatio = 1.0;
			//}

			double aspectRatio = 1.0;

			// Set the fullscale width and height in the X1-X2 coordinate system
			UINT fullScaleWidth = UINT(_pSrcData->GetHeight() * aspectRatio);		// Resample the aline axis
			UINT fullScaleHeight = _pSrcData->GetHeight();							// no change


			// Acquistion axis are oreientation independent
			_pAxisRhoInfo = new AxisInfo(pSource->GetAxis1Info());					// ALine axis
			_pAxisThetaInfo = new AxisInfo(pSource->GetAxis2Info());				// Theta axis

			assert(_pAxisRhoInfo->GetType() == AxisType::Length);
			assert(_pAxisThetaInfo->GetType() == AxisType::Angle);

			_aquitionIndexRangeX[0] = 0;						// inclusive
			_aquitionIndexRangeX[1] = _pSrcData->GetWidth();	// exclusive
			_aquitionIndexRangeY[0] = 0;
			_aquitionIndexRangeY[1] = _pSrcData->GetHeight();


			// Local axes depend on orinetation
			double screenRho = _pAxisRhoInfo->GetLast();
			_pAxisXInfo = new AxisInfo(-screenRho, screenRho, AxisType::Length, _pAxisRhoInfo->GetUnits());
			_pAxisYInfo = new AxisInfo(-screenRho, screenRho, AxisType::Length, _pAxisRhoInfo->GetUnits());
			_fullScaleWidth = fullScaleHeight;
			_fullScaleHeight = fullScaleWidth;


			if (_orientation & Orientation::FlipLeftRight)
			{
				_pAxisXInfo->SwapFirstLast();
				//SWAP(_aquitionIndexRangeX[0], _aquitionIndexRangeX[1], int);
			}
			if (_orientation & Orientation::FlipTopBottom)
			{
				_pAxisYInfo->SwapFirstLast();
				//SWAP(_aquitionIndexRangeY[0], _aquitionIndexRangeY[1], int);
			}


			//// Transform into the local X-Y coordinate system
			//if (_orientation & Orientation::Transpose)
			//{
			//	_pAxisXInfo = new AxisInfo(pSource->GetAxis2Info());
			//	_pAxisYInfo = new AxisInfo(pSource->GetAxis1Info());
			//	_fullScaleWidth = fullScaleHeight;
			//	_fullScaleHeight = fullScaleWidth;
			//	_aquitionIndexRangeX[0] = 0;
			//	_aquitionIndexRangeX[1] = _pSrcData->GetHeight();
			//	_aquitionIndexRangeY[0] = 0;
			//	_aquitionIndexRangeY[1] = _pSrcData->GetWidth();
			//}
			//else
			//{
			//	_pAxisXInfo = new AxisInfo(pSource->GetAxis1Info());
			//	_pAxisYInfo = new AxisInfo(pSource->GetAxis2Info());
			//	_fullScaleWidth = fullScaleWidth;
			//	_fullScaleHeight = fullScaleHeight;
			//	_aquitionIndexRangeX[0] = 0;
			//	_aquitionIndexRangeX[1] = _pSrcData->GetWidth();
			//	_aquitionIndexRangeY[0] = 0;
			//	_aquitionIndexRangeY[1] = _pSrcData->GetHeight();
			//}
			//if (_orientation & Orientation::FlipLeftRight)
			//{
			//	_pAxisXInfo->SwapFirstLast();
			//	SWAP(_aquitionIndexRangeX[0], _aquitionIndexRangeX[1], int);
			//}
			//if (_orientation & Orientation::FlipTopBottom)
			//{
			//	_pAxisYInfo->SwapFirstLast();
			//	SWAP(_aquitionIndexRangeY[0], _aquitionIndexRangeY[1], int);
			//}

		}

		R1T2RenderTarget::~R1T2RenderTarget()
		{
			delete(_pAxisRhoInfo);
			delete(_pAxisThetaInfo);
		}


		void R1T2RenderTarget::OnPositionChanged(ACQUISTIONPOINT oldPoint, ACQUISTIONPOINT newPoint, RenderTarget* pInitiator)
		{
			BOOL rhoChanged = (newPoint.X1 != oldPoint.X1);
			BOOL thetaChanged = (newPoint.X2 != oldPoint.X2);
			BOOL ContentChanged = (newPoint.X3 != oldPoint.X3);

			PositionChangedEventArgs args = PositionChangedEventArgs(oldPoint, newPoint);
			args.Rho = AcquitionIndexToRho(newPoint.X1);
			args.Theta = AcquitionIndexToTheta(newPoint.X2);

			// Send events
			if (rhoChanged)
				OnCurrentPositionChangedRho(args);

			if (thetaChanged)
				OnCurrentPositionChangedTheta(args);

			if (ContentChanged)
				OnContentChanged();
		}


		ACQUISTIONPOINT R1T2RenderTarget::ConvertScreenCoordinatesToAcquistionIndices(int x, int y)
		{
			ACQUISTIONPOINT p;

			double xp = double(x) - double(_width) / 2.0;
			double yp = double(y) - double(_height) / 2.0;

			double rho = sqrt(xp*xp + yp*yp);
			double normRho = ScreenRhoToNormalizedScreenRho(rho);
			p.X1 = NormalizedScreenRhoToAcquitionIndex(normRho);

			double theta = atan2(yp, xp);
			double normTheta = ScreenThetaToNormalizedScreenTheta(theta);
			p.X2 = NormalizedScreenThetaToAcquitionIndex(normTheta);


			p.X3 = _pProcessor->GetX3();


			return p;
		}

		void R1T2RenderTarget::SetCurrentRho(double value)
		{
			int X1 = NormalizedScreenRhoToAcquitionIndex(value);
			_pProcessor->SetX1(X1, this);
		}

		double R1T2RenderTarget::GetCurrentRho()
		{
			int X1 = _pProcessor->GetX1();
			double rho = AcquitionIndexToRho(X1);
			return rho;
		}

		void R1T2RenderTarget::SetCurrentTheta(double value)
		{
			double normTheta = ScreenThetaToNormalizedScreenTheta(value);
			int X2 = NormalizedScreenThetaToAcquitionIndex(normTheta);
			_pProcessor->SetX2(X2, this);
		}

		double R1T2RenderTarget::GetCurrentTheta()
		{
			int X2 = _pProcessor->GetX2();
			double theta = AcquitionIndexToTheta(X2);
			return theta;
		}


		void R1T2RenderTarget::Render(void * pResource, bool isNewSurface)
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
			pGpuProcessor->RenderPolar(_pRenderTargetView, &_viewPort, _orientation);
			
		}

		// CATHETER POSITION

		CATHETERINFO R1T2RenderTarget::GetCatheterInfo()
		{
			// Get position from source
			OckFileSource* pSrc = _pProcessor->GetSource();
			double diameter = pSrc->GetCatheterDiameter();				// mm
			double edgePosition = pSrc->GetCatheterEdgePosition();		// mm
			double offsetPosition = pSrc->GetALineOffset();				// mm

			AxisInfo alineAxis = pSrc->GetAxis1Info();
			
			CatheterEdgeProjection projection = CatheterEdgeProjection::CenteredCircle;
			double normalizedRadius = (edgePosition - offsetPosition) / (alineAxis.GetLast() - offsetPosition);
			int screenRadius = NormalizedScreenYToScreenY(normalizedRadius);

			CATHETERINFO info =CATHETERINFO(diameter, projection);
			info.ScreenDiameter = 2 * screenRadius;

			return info;
		}


		void R1T2RenderTarget::SetProposedCatheterEdgePosition(double normalizedPosition)
		{
			// Set value on processor
			// Processor will notify all render targets

			// Convert from offset-corrected normalized aline position to mm of air
			OckFileSource* pSrc = _pProcessor->GetSource();
			double offsetPosition = pSrc->GetALineOffset();
			AxisInfo alineAxis = pSrc->GetAxis1Info();

			double position = normalizedPosition*(alineAxis.GetDelta() - offsetPosition) + alineAxis.GetFirst();

			_pProcessor->SetProposedCatheterEdgePosition(position);
		}

		double R1T2RenderTarget::GetProposedCatheterEdgePosition()
		{
			double position = _pProcessor->GetProposedCatheterEdgePosition();
			// Convert from  mm of air to normalized aline position
			OckFileSource* pSrc = _pProcessor->GetSource();
			AxisInfo alineAxis = pSrc->GetAxis1Info();
			double offsetPosition = pSrc->GetALineOffset();
			double normalizedPosition = (position - alineAxis.GetFirst()) / (alineAxis.GetDelta() - offsetPosition);

			return normalizedPosition;
		}

		void R1T2RenderTarget::CommitProposedCatheterEdgePosition()
		{
			// TODO
		}

		void R1T2RenderTarget::NotifyCatheterEdgePositionChanged(double position, RenderTarget* pInitiator)
		{
			// Notify container hosting the render target that the catheter needs to be redrawn at a new position

			// Convert from  mm of air to normalized aline position
			OckFileSource* pSrc = _pProcessor->GetSource();
			AxisInfo alineAxis = pSrc->GetAxis1Info();
			double offsetPosition = pSrc->GetALineOffset();
			double normalizedPosition = (position - alineAxis.GetFirst()) / (alineAxis.GetDelta() - offsetPosition);

			PositionChangedEventArgs args;
			args.Rho = normalizedPosition;
			OnCatheterEdgePositionChanged(args);
		}

		double R1T2RenderTarget::GetNormalizedCatheterEdgePosition()
		{
			// Return normalized position of catheter edge
			// Position is relative to the offset (not the start) of the raw aline and normalized to its offset length (not full length)

			OckFileSource* pSrc = _pProcessor->GetSource();
			AxisInfo alineAxis = pSrc->GetAxis1Info();
			double offsetPosition = pSrc->GetALineOffset();
			double value = (pSrc->GetCatheterEdgePosition() - offsetPosition) / (alineAxis.GetLast() - offsetPosition);

			return value;
		}

		void R1T2RenderTarget::SetNormalizedCatheterEdgePosition(double value)
		{
			// Set then normalized position of catheter edge
			// Position is relative to the offset (not the start) of the raw aline and normalized to its offset length (not full length)
			// The size of the catheter does not change. The offset is changed so the catheter edge appears to to move

			// Find value in mm of air
			OckFileSource* pSrc = _pProcessor->GetSource();
			AxisInfo alineAxis = pSrc->GetAxis1Info();
			double offsetPosition = pSrc->GetALineOffset();
			double catheterEdgePosition = value * (alineAxis.GetLast() - offsetPosition) + offsetPosition;	

			// Notify the processor
			// Processor will notify all render targets
			//_pProcessor->SetProposedCatheterEdgePosition(catheterEdgePosition, this);

		}

		double R1T2RenderTarget::GetNormalizedOffsetPosition()
		{
			// Return normalized position of aline offset
			// Position is relative to the offset (not the start) of the raw aline and normalized to its offset length (not full length)

			// Must be zero by definition 
			return 0.0;
		}

		
	} // namespace proc
} // namespace oct