#pragma once

#include "PolarRenderTarget.h"


namespace oct {
	namespace proc {


		// Forwatd defintion for Processor
		class Processor;

		class R1T2RenderTarget : public PolarRenderTarget
		{

		public:

			R1T2RenderTarget(Processor* const pProcessor, Orientation orientation);
			virtual ~R1T2RenderTarget();

			virtual void Render(void * pResource, bool isNewSurface);
			
			virtual void SetCurrentRho(double value);
			virtual void SetCurrentTheta(double value);

			virtual double GetCurrentRho();
			virtual double GetCurrentTheta();

			
			//virtual POINT GetCurrentPoint();
	
			virtual ACQUISTIONPOINT ConvertScreenCoordinatesToAcquistionIndices(int x, int y);

			virtual void OnPositionChanged(ACQUISTIONPOINT oldPoint, ACQUISTIONPOINT newPoint, RenderTarget* pInitiator);


			// Overrides
			virtual double GetNormalizedOffsetPosition();
			virtual double GetNormalizedCatheterEdgePosition();
			virtual void SetNormalizedCatheterEdgePosition(double value);


			// Catheter position
			virtual CATHETERINFO GetCatheterInfo();
			virtual void NotifyCatheterEdgePositionChanged(double value, RenderTarget* pInitiator);
			virtual void SetProposedCatheterEdgePosition(double value);
			virtual double GetProposedCatheterEdgePosition();
			virtual void CommitProposedCatheterEdgePosition();

		protected:


		};

	} // namespace proc
} // namespace oct