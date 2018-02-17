#pragma once

//#include "RenderTarget.h"
//#include "ApodizationWindow.h"
//#include "DataResource2D.h"

namespace oct {
	namespace proc {

		class PerThreadData
		{

		public:
			PerThreadData(int thread, int N, int ExternalWorkBufferSize);
			~PerThreadData();

		public:
			int _thread;
			Ipp32f *_pBuf0;
			Ipp32f *_pBuf1;
			Ipp8u *_pExternalWorkBuffer;	 // For FFT function
		};


		class ProcessDataManager
		{
			//friend class CpuProcessor;

		public:
			ProcessDataManager(int fftOrder);
			~ProcessDataManager();

			PerThreadData *GetPerThreadData(int threadId)
			{
				return _pPerThreadData[threadId];
			}

			int GetThreadCount()
			{
				return _threadCount;
			}
			int GetMaxThreadCount()
			{
				return _maxThreadCount;
			}
			void SetThreadCount(int threadCount)
			{
				_threadCount = min(threadCount, _maxThreadCount);
			}
			IppsFFTSpec_R_32f *GetSpecificationStructure()
			{
				return _pSpec;
			}

		private:

			// Shared FFT specifcation structure and memory
			IppsFFTSpec_R_32f *_pSpec;
			Ipp8u *_pMemSpec;
	
			// Per-thread data
			int _threadCount;
			int _maxThreadCount;
			PerThreadData **_pPerThreadData;

		};

			

	} // namespace proc
} // namespace oct