#include "stdafx.h"
#include "ProcessDataManager.h"
#include <omp.h>

namespace oct {
	namespace proc {

		PerThreadData::PerThreadData(int thread, int fftOrder, int ExternalWorkBufferSize)
		{
			int N = 1 << fftOrder;

			_thread = thread;
			_pBuf0 = ippsMalloc_32f(N + 2);
			_pBuf1 = ippsMalloc_32f(N + 2);
			_pExternalWorkBuffer = (ExternalWorkBufferSize > 0) ? ippsMalloc_8u(ExternalWorkBufferSize) : NULL;
		}

		PerThreadData::~PerThreadData()
		{
			ippsFree(_pBuf0);
			ippsFree(_pBuf1);
			ippsFree(_pExternalWorkBuffer);
		}

		ProcessDataManager::ProcessDataManager(int fftOrder)
		{
			IppStatus status;
			const int flag = IPP_FFT_DIV_INV_BY_N;
			const IppHintAlgorithm hint = ippAlgHintNone;

			// FFT specification structure (shared by all threads)
			Ipp8u *pMemInit = NULL;
			int sizeSpec, sizeInit, sizeBuffer;
			status = ippsFFTGetSize_C_32fc(fftOrder, flag, hint, &sizeSpec, &sizeInit, &sizeBuffer);
			_pMemSpec = ippsMalloc_8u(sizeSpec);
			pMemInit = (sizeInit > 0) ? ippsMalloc_8u(sizeInit) : NULL;
			status = ippsFFTInit_R_32f(&_pSpec, fftOrder, flag, hint, _pMemSpec, pMemInit);
			if (pMemInit) ippsFree(pMemInit);

			// Thread count
			_maxThreadCount = omp_get_max_threads();
			_threadCount = _maxThreadCount;

			// Create per-thread processing buffers for the max number of threads
			_pPerThreadData = new PerThreadData*[_maxThreadCount];
			for (int thread = 0; thread < _maxThreadCount; thread++)
			{
				_pPerThreadData[thread] = new PerThreadData(thread, fftOrder, sizeBuffer);
			}
		}

		ProcessDataManager::~ProcessDataManager()
		{
			// Fee memory used by FFT specification structure
			ippsFree(_pMemSpec);

			// Delete per-thread processing buffers
			for (int thread = 0; thread < _maxThreadCount; thread++)
			{
				delete _pPerThreadData[thread];
			}
		}
		

	} // namespace proc
} // namespace oct

	