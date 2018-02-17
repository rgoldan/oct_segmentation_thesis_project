#pragma once

#include "ColormapArrayResource.h"
#include "Definitions.h"

namespace oct {
	namespace proc {


		// Forward defs
		class RenderTarget;


		struct ColorChangedEventArgs
		{
		public:
			ColorChangedEventArgs() {};
			ColorChangedEventArgs(float _Black, float _White, ColormapType _Colormap) :
				Black(_Black), White(_White), Colormap(_Colormap) { };
			ColorChangedEventArgs(RenderTarget *pRenderTarget);
			float Black = 0.0f;
			float White = 0.0f;
			ColormapType Colormap = static_cast<ColormapType>(0);
		};



		struct PositionChangedEventArgs
		{
		public:
			PositionChangedEventArgs() {};
			PositionChangedEventArgs(double _Rho) : Rho(_Rho) {};
			PositionChangedEventArgs(ACQUISTIONPOINT _OldPoint, ACQUISTIONPOINT _NewPoint) :
				OldPoint(_OldPoint), NewPoint(_NewPoint) { };
			ACQUISTIONPOINT OldPoint = { 0,0,0 };
			ACQUISTIONPOINT NewPoint = { 0,0,0 };
			int X = 0;
			int Y = 0;
			double Rho = 0.0;
			double Theta = 0.0;
		};

		

	} // namespace proc
} // namespace oct