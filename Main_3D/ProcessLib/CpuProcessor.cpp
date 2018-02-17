#include "stdafx.h"
#include "CpuProcessor.h"
#include "Utility.h"
#include "ProcessDataManager.h"
#include <omp.h>

namespace oct {
	namespace proc {


		CpuProcessor::CpuProcessor()
		{
			
			QueryPerformanceFrequency(&_tickFrequeny);
		}


		CpuProcessor::~CpuProcessor()
		{
			SAFE_DELETE(_pWindow);
			SAFE_DELETE(_pProcessData);
		}

		int CpuProcessor::GetThreadCount()
		{
			return _pProcessData->GetThreadCount();
		}
		void CpuProcessor::SetThreadCount(int threadCount)
		{
			_pProcessData->SetThreadCount(threadCount);
		}

		void CpuProcessor::SetKSpaceSamplesPerALine(int kSpaceSamplesPerALine)
		{
			if (kSpaceSamplesPerALine != _kSpaceSamplesPerALine)
			{

				_fftOrder = (int)log2(kSpaceSamplesPerALine) + 1;
				_samplesPerALine = (int)pow(2, _fftOrder - 1);

				SAFE_DELETE(_pProcessData);
				_pProcessData = new ProcessDataManager(_fftOrder);

				SAFE_DELETE(_pWindow);
				_pWindow = new ApodizationWindow(kSpaceSamplesPerALine);

				_kSpaceSamplesPerALine = kSpaceSamplesPerALine;
			}
		}


		void CpuProcessor::ProcessALines(const signed short * const pSrc0, int srcStep, int kSpaceSamplesPerALine, int aLineCount, float * const pDst0, int dstStep)
		{

			assert(kSpaceSamplesPerALine == _kSpaceSamplesPerALine);
			
			// Shared vars (all variables at function scope are shared by default)
			int N = 1 << _fftOrder;
			const IppsFFTSpec_R_32f *pSpec = _pProcessData->GetSpecificationStructure();
			const Ipp32f vSub = log10(_referenceLevel);
			const Ipp32f vDiv = Ipp32f(1.0/20.0);
			const int desiredThreadCount = _pProcessData->GetThreadCount();
			LARGE_INTEGER t0, t1;

			QueryPerformanceCounter(&t0);

		#pragma omp parallel num_threads(desiredThreadCount) default(shared)
			{

				// Private (per thread) vars
				IppStatus status;

				const int threadId = omp_get_thread_num();
				const int threadCount = omp_get_num_threads();
				const int n0 = threadId * aLineCount / threadCount;
				const int n1 = (threadId + 1)*aLineCount / threadCount;

				const Ipp16s *pSrc = pSrc0 + n0*(srcStep >> 1);						// Ipp16s
				Ipp32f *pDst = pDst0 + n0*(dstStep >> 2);							// Ipp32f

				PerThreadData *pPTD = _pProcessData->GetPerThreadData(threadId);

				for (int n = n0; n < n1; n++)
				{
					// Convert to float
					ippsConvert_16s32f(pSrc, pPTD->_pBuf0, kSpaceSamplesPerALine);

					// Apply Window
					if (_pWindow->GetType() != ApodizationWindowType::None)
						ippsMul_32f_I(_pWindow->GetDataPtr(), pPTD->_pBuf0, kSpaceSamplesPerALine);

					// Zero Pad (set remaining samples set to zero)
					ippsZero_32f(pPTD->_pBuf0 + kSpaceSamplesPerALine, N - kSpaceSamplesPerALine);

					// FFT
					status = ippsFFTFwd_RToCCS_32f(pPTD->_pBuf0, pPTD->_pBuf1, pSpec, pPTD->_pExternalWorkBuffer);
					assert(status == ippStsNoErr);

					// Magnitude
					status = ippsMagnitude_32fc((Ipp32fc*)pPTD->_pBuf1, pPTD->_pBuf0, _samplesPerALine);
					assert(status == ippStsNoErr);

					// Linear to db conversion
					//status = ippsLog10_32f_A11(pPTD->_pBuf0, pDst, _samplesPerALine);	// 11 correctly rounded bits of significand, or at least 3 exact decimal digits
					//status = ippsLog10_32f_A21(pPTD->_pBuf0, pDst, _samplesPerALine);	// 21 correctly rounded bits of significand, or about 6 exact decimal digits
					status = ippsLog10_32f_A24(pPTD->_pBuf0, pDst, _samplesPerALine);	// 24 correctly rounded bits of significand, including the implied bit, with the maximum guaranteed error within 1 ulp.
					assert(status == ippStsNoErr);
					status = ippsNormalize_32f_I(pDst, _samplesPerALine, vSub, vDiv);
					assert(status == ippStsNoErr);

					// Next ALine
					pSrc += srcStep >> 1;			// Ipp16s
					pDst += dstStep >> 2;			// Ipp32f
				}

			}
					
			// Save performance info
			QueryPerformanceCounter(&t1);
			_tickCounter.QuadPart += t1.QuadPart - t0.QuadPart;
			_aLineCounter += aLineCount;
		}


		int CpuProcessor::GetPerformanceALinesPerSecond()
		{
			LARGE_INTEGER aLinesPerSecond;

			if (_tickCounter.QuadPart)
			{

				aLinesPerSecond.QuadPart = _aLineCounter*_tickFrequeny.QuadPart;
				aLinesPerSecond.QuadPart /= _tickCounter.QuadPart;
			}
			else
			{
				aLinesPerSecond.QuadPart = 0;
			}

			return static_cast<int>(aLinesPerSecond.QuadPart);
		}

		int CpuProcessor::GetPerformanceBytesPerSecond()
		{
			int bytesPerSecond = GetPerformanceALinesPerSecond();
			bytesPerSecond *= 2 * _kSpaceSamplesPerALine;
			return bytesPerSecond;
		}

		void CpuProcessor::ResetPerformanceCounters()
		{
			_aLineCounter = 0;
			_tickCounter.QuadPart = 0;
		}


	} // namespace proc
} // namespace oct

	