#include "stdafx.h"
#include "Utility.h"
#include "CartesianRenderTarget.h"


using namespace DirectX;
using namespace DirectX::PackedVector;



namespace oct {
	namespace proc {

		CartesianRenderTarget::CartesianRenderTarget(Processor* const pProcessor, Orientation orientation) : RenderTarget(pProcessor, orientation)
		{
		}

		CartesianRenderTarget::~CartesianRenderTarget()
		{
		}

		void CartesianRenderTarget::OnCurrentPositionChangedX(PositionChangedEventArgs args)
		{
			__raise CurrentPositionChangedXEvent(this, args);
		}

		void CartesianRenderTarget::OnCurrentPositionChangedY(PositionChangedEventArgs args)
		{
			__raise CurrentPositionChangedYEvent(this, args);
		}

		void CartesianRenderTarget::OnCatheterEdgePositionChanged(PositionChangedEventArgs args)
		{
			__raise CatheterEdgePositionChangedEvent(this, args);
		}

		void CartesianRenderTarget::OnContentChanged()
		{
			__raise ContentChangedEvent(this);
		}

		void CartesianRenderTarget::OnLevelsChanged(ColorChangedEventArgs args)
		{
			__raise LevelsChangedEvent(this, args);
		}

		void CartesianRenderTarget::OnColormapChanged(ColorChangedEventArgs args)
		{
			__raise ColormapChangedEvent(this, args);
		}


	} // namespace proc
} // namespace oct