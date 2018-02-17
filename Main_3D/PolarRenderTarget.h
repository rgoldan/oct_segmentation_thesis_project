#pragma once

#include "RenderTarget.h"


namespace oct {
	namespace proc {

		// Forwatd defintion for Processor
		class Processor;

		[event_source(native)]
		class PolarRenderTarget : public RenderTarget
		{

		public:

			PolarRenderTarget(Processor* const pProcessor, Orientation orientation);
			virtual ~PolarRenderTarget();

			virtual void SetCurrentRho(double value) = 0;
			virtual void SetCurrentTheta(double value) = 0;
			//virtual POINT GetCurrentPoint() = 0;

			virtual double GetCurrentRho() = 0;
			virtual double GetCurrentTheta() = 0;

			// EVENTS
			__event void CurrentPositionChangedRhoEvent(PolarRenderTarget *obj, PositionChangedEventArgs args);
			__event void CurrentPositionChangedThetaEvent(PolarRenderTarget *obj, PositionChangedEventArgs args);
			__event void ContentChangedEvent(PolarRenderTarget *obj);
			__event void ColormapChangedEvent(PolarRenderTarget *obj, ColorChangedEventArgs args);
			__event void LevelsChangedEvent(PolarRenderTarget *obj, ColorChangedEventArgs args);

			__event void CatheterEdgePositionChangedEvent(PolarRenderTarget *obj, PositionChangedEventArgs args);


			virtual void OnCurrentPositionChangedRho(PositionChangedEventArgs args);
			virtual void OnCurrentPositionChangedTheta(PositionChangedEventArgs args);
			virtual void OnContentChanged();
			virtual void OnColormapChanged(ColorChangedEventArgs args);
			virtual void OnLevelsChanged(ColorChangedEventArgs args);

			virtual void OnCatheterEdgePositionChanged(PositionChangedEventArgs args);

		protected:

			double ScreenRhoToNormalizedScreenRho(double value);

			int NormalizedScreenRhoToAcquitionIndex(double value);

			double ScreenThetaToNormalizedScreenTheta(double value);

			int NormalizedScreenThetaToAcquitionIndex(double value);

			double AcquitionIndexToRho(int value);
			double AcquitionIndexToTheta(int value);

		protected:

			AxisInfo*				_pAxisRhoInfo = NULL;
			AxisInfo*				_pAxisThetaInfo = NULL;


		};

	} // namespace proc
} // namespace oct