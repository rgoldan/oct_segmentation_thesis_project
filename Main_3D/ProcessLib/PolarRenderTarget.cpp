#include "stdafx.h"
#include "Utility.h"
#include "PolarRenderTarget.h"


using namespace DirectX;
using namespace DirectX::PackedVector;



namespace oct {
	namespace proc {

		PolarRenderTarget::PolarRenderTarget(Processor* const pProcessor, Orientation orientation) : RenderTarget(pProcessor, orientation)
		{
		}

		PolarRenderTarget::~PolarRenderTarget()
		{
		}

		void PolarRenderTarget::OnCurrentPositionChangedRho(PositionChangedEventArgs args)
		{
			__raise CurrentPositionChangedRhoEvent(this, args);
		}

		void PolarRenderTarget::OnCurrentPositionChangedTheta(PositionChangedEventArgs args)
		{
			__raise CurrentPositionChangedThetaEvent(this, args);
		}

		void PolarRenderTarget::OnCatheterEdgePositionChanged(PositionChangedEventArgs args)
		{
			__raise CatheterEdgePositionChangedEvent(this, args);
		}

		void PolarRenderTarget::OnContentChanged()
		{
			__raise ContentChangedEvent(this);
		}

		void PolarRenderTarget::OnLevelsChanged(ColorChangedEventArgs args)
		{
			__raise LevelsChangedEvent(this, args);
		}

		void PolarRenderTarget::OnColormapChanged(ColorChangedEventArgs args)
		{
			__raise ColormapChangedEvent(this, args);
		}

		double PolarRenderTarget::ScreenRhoToNormalizedScreenRho(double value)
		{
			double screenRhoMax = double(_width) / 2.0 + 1;
			double normScreenRho = value / screenRhoMax;
			return CLAMP(normScreenRho, 0.0, 1.0);;
		}

		int PolarRenderTarget::NormalizedScreenRhoToAcquitionIndex(double value)
		{
			int X1 = _aquitionIndexRangeX[0] + static_cast<int>(value*(_aquitionIndexRangeX[1] - _aquitionIndexRangeX[0]));
			return CLAMP(X1, _aquitionIndexRangeX[0], _aquitionIndexRangeX[1]-1);
		}

		double PolarRenderTarget::ScreenThetaToNormalizedScreenTheta(double value)
		{
			// Expect -pi <= value < pi
			return (value + IPP_PI) / IPP_2PI;
		}

		int PolarRenderTarget::NormalizedScreenThetaToAcquitionIndex(double value)
		{
			return _aquitionIndexRangeY[0] + static_cast<int>(value*(_aquitionIndexRangeY[1] - _aquitionIndexRangeY[0]));
		}


		double PolarRenderTarget::AcquitionIndexToRho(int value)
		{
			double norm = double(value) / double(_aquitionIndexRangeX[1]);
			//double screenRhoMax = double(_width) / 2.0 + 1;
			return norm;
		}


		double PolarRenderTarget::AcquitionIndexToTheta(int value)
		{
			double norm = double(value) / double(_aquitionIndexRangeY[1]);
			return norm*IPP_2PI - IPP_PI;
		}



	} // namespace proc
} // namespace oct