#pragma once

#include "CartesianRenderTarget.h"


namespace oct {
	namespace proc {


		// Forwatd defintion for Processor
		class Processor;

		class X1X2RenderTarget : public CartesianRenderTarget
		{

		public:

			X1X2RenderTarget(Processor* const pProcessor, Orientation orientation);
			virtual ~X1X2RenderTarget();

			virtual void Render(void * pResource, bool isNewSurface);
			
			virtual void SetCurrentPointX(int value);
			virtual void SetCurrentPointY(int value);
			virtual POINT GetCurrentPoint();
	
			virtual ACQUISTIONPOINT ConvertScreenCoordinatesToAcquistionIndices(int x, int y);

			virtual void OnPositionChanged(ACQUISTIONPOINT oldPoint, ACQUISTIONPOINT newPoint, RenderTarget* pInitiator);

			// Catheter position
			virtual CATHETERINFO GetCatheterInfo();
			virtual void NotifyCatheterEdgePositionChanged(double position, RenderTarget* pInitiator);
			virtual void SetProposedCatheterEdgePosition(double normalizedPosition);
			virtual double GetProposedCatheterEdgePosition();
			virtual void CommitProposedCatheterEdgePosition();

		protected:


		};

	} // namespace proc
} // namespace oct