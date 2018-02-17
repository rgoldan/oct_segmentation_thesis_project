#pragma once

#include "RenderTarget.h"


namespace oct {
	namespace proc {

		// Forwatd defintion for Processor
		class Processor;

		[event_source(native)]
		class CartesianRenderTarget : public RenderTarget
		{

		public:

			CartesianRenderTarget(Processor* const pProcessor, Orientation orientation);
			virtual ~CartesianRenderTarget();

			virtual void SetCurrentPointX(int value) = 0;
			virtual void SetCurrentPointY(int value) = 0;
			virtual POINT GetCurrentPoint() = 0;
			
			
			// EVENTS
			__event void CurrentPositionChangedXEvent(CartesianRenderTarget *obj, PositionChangedEventArgs args);
			__event void CurrentPositionChangedYEvent(CartesianRenderTarget *obj, PositionChangedEventArgs args);
			__event void ContentChangedEvent(CartesianRenderTarget *obj);
			__event void ColormapChangedEvent(CartesianRenderTarget *obj, ColorChangedEventArgs args);
			__event void LevelsChangedEvent(CartesianRenderTarget *obj, ColorChangedEventArgs args);

			__event void CatheterEdgePositionChangedEvent(CartesianRenderTarget *obj, PositionChangedEventArgs args);



			virtual void OnCurrentPositionChangedX(PositionChangedEventArgs args);
			virtual void OnCurrentPositionChangedY(PositionChangedEventArgs args);
			virtual void OnContentChanged();
			virtual void OnColormapChanged(ColorChangedEventArgs args);
			virtual void OnLevelsChanged(ColorChangedEventArgs args);

			virtual void OnCatheterEdgePositionChanged(PositionChangedEventArgs args);
			

		private:


		};

	} // namespace proc
} // namespace oct