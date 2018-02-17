#pragma once

#include "Utility.h"
#include "GpuProcessor.h"
#include "CpuProcessor.h"
#include "OckFileSource.h"


namespace oct {
	namespace proc {

			
		class Processor
		{

		public:

			Processor();
			~Processor();

			void SetSource(OckFileSource *pSource);

			GpuProcessor* GetGpuProcessor()
			{
				return _pGpuProcessor;
			}

			int GetCpuPerformanceALinesPerSecond()
			{
				return _pCpuProcessor->GetPerformanceALinesPerSecond();
			}
			int GetCpuPerformanceBytesPerSecond()
			{
				return _pCpuProcessor->GetPerformanceBytesPerSecond();
			}
			void ResetCpuPerformanceCounters()
			{
				_pCpuProcessor->ResetPerformanceCounters();
			}

			int GetLengthX1() { return _pCpuProcessor->GetSamplesPerALine(); };
			int GetLengthX2() { return _pSource->GetAxis2Length(); };
			int GetLengthX3() { return _pSource->GetAxis3Length(); };

			int GetX1() { return _currentPoint.X1; };
			int GetX2() { return _currentPoint.X2; };
			int GetX3() { return _currentPoint.X3; };


			void SetX1(int value, RenderTarget* pInitiator = NULL);
			void SetX2(int value, RenderTarget* pInitiator = NULL);
			void SetX3(int value, RenderTarget* pInitiator = NULL);

			void SetProposedCatheterEdgePosition(double position, RenderTarget* pInitiator = NULL);
			double GetProposedCatheterEdgePosition() { return _proposedCatheterEdgePosition; };

			BOOL HaveSource()
			{
				return _pSource!=NULL;
			};

			OckFileSource* GetSource()
			{
				return _pSource;
			};

		private:

			ACQUISTIONPOINT _currentPoint = { 0,0,0 };

			//UINT _X1 = 0;
			//UINT _X2 = 0;
			//UINT _X3 = 0;

			CpuProcessor*	_pCpuProcessor = NULL;
			GpuProcessor*	_pGpuProcessor = NULL;

			OckFileSource*	_pSource = NULL;
			float*			_pBuf = NULL;
			int				_bufPitch = 0;

			double			_proposedCatheterEdgePosition = 0.0;

		};

	} // namespace proc
} // namespace oct