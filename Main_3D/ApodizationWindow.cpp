#include "stdafx.h"
#include "ApodizationWindow.h"

namespace oct {
	namespace proc {

		ApodizationWindow::ApodizationWindow(int length) :
			ApodizationWindow(length, ApodizationWindowType::Hann) {}

		ApodizationWindow::ApodizationWindow(int length, ApodizationWindowType type)
		{
			_type = type;
			_length = length;
			_alpha = 2.0;	// Kaiser parameter

			// Allocate memory for window function and build
			_pData = ippsMalloc_32f(length);
			Build();

		}


		ApodizationWindow::~ApodizationWindow()
		{
			ippsFree(_pData);
		}


		void ApodizationWindow::SetType(ApodizationWindowType type)
		{
			_type = type;
			Build();
		}


		void ApodizationWindow::Build()
		{
			IppStatus status = ippsSet_32f(1, _pData, _length);
			assert(status == ippStsNoErr);

			switch (_type)
			{
			case ApodizationWindowType::None:
				break;
			case ApodizationWindowType::Bartlett:
				status = ippsWinBartlett_32f_I(_pData, _length);
				break;
			case ApodizationWindowType::Blackman:
				status = ippsWinBlackmanStd_32f_I(_pData, _length);
				break;
			case ApodizationWindowType::Hamming:
				status = ippsWinHamming_32f_I(_pData, _length);
				break;
			case ApodizationWindowType::Hann:
				status = ippsWinHann_32f_I(_pData, _length);
				break;
			case ApodizationWindowType::Kaiser:
			{
				// IPP uses a different definition of alpha so convert first
				float ippAlpha = _alpha * 2 * (float)M_PI / (_length - 1);
				status = ippsWinKaiser_32f_I(_pData, _length, ippAlpha);
			}
			break;
			default:
				assert(FALSE);
				break;

				// Check for errors before returning
				assert(status == ippStsNoErr);
			}
		}

	} // namespace proc
} // namespace oct