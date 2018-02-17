#pragma once

#include "ApodizationWindow.h"

namespace oct {
	namespace proc {


		enum class Polarization
		{
			Undefined		= 0,
			ChannelA		= 1,
			ChannelB		= 2,
			PDD				= 3,
			PS				= 4,
		};

		// Forward definitions
		class ProcessDataManager;

		class CpuProcessor
		{

		public:
			CpuProcessor();
			~CpuProcessor();

		private:

			int					_fftOrder = 0;
			int					_samplesPerALine = 0;
			int					_kSpaceSamplesPerALine = 0;
			Polarization		_polarizationAlgorithm = Polarization::ChannelA;
			int					_filterKernel = 0;
			float				_referenceLevel = 0;

			ProcessDataManager	*_pProcessData = NULL;
			ApodizationWindow	*_pWindow = NULL;

			int					_aLineCounter = 0;
			LARGE_INTEGER		_tickCounter = {0,0};
			LARGE_INTEGER		_tickFrequeny = {0,0};


		public:

			void ProcessALines(const signed short * const pSrc0, int srcStep, int kSpaceSamplesPerALine, int aLineCount, float * const pDst0, int dstStep);

			void SetKSpaceSamplesPerALine(int kSpaceSamplesPerALine);

			int GetSamplesPerALine() { return _samplesPerALine; }

			void SetApodizationWindow(ApodizationWindowType type) {
				_pWindow->SetType(type);
			}
			ApodizationWindowType GetApodizationWindowType() {
				return _pWindow->GetType();
			}

			int GetPerformanceALinesPerSecond();
			int GetPerformanceBytesPerSecond();
			void ResetPerformanceCounters();

			int GetThreadCount();
			void SetThreadCount(int threadCount);
			void SetReferenceLevel(float referenceLevel)
			{
				_referenceLevel = referenceLevel;
			}
			float GetReferenceLevel()
			{
				return _referenceLevel;
			}

		private:



		};

	} // namespace proc
} // namespace oct