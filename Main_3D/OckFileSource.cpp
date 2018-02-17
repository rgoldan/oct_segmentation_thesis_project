#include "stdafx.h"
#include "OckFileSource.h"



namespace oct {
	namespace proc {


		DataAcqusitionSettings::DataAcqusitionSettings()
		{
			ZeroMemory(this, sizeof(DataAcqusitionSettings));
		}

		DataAcqusitionSettings::~DataAcqusitionSettings()
		{
		};



		OckFileSource::OckFileSource(const char *filename)
		{
			_hFile = CreateFileA(
				filename,					// Filename
				GENERIC_READ,				// Access
				0,							// Share mode
				NULL,						// Security attributes
				OPEN_EXISTING,				// Creation disposition
				FILE_ATTRIBUTE_NORMAL,		// Flags
				NULL						// Template file
				);

			// Check to see if file was opened
			if (_hFile == INVALID_HANDLE_VALUE)
			{
				// File does not exist
				char msg[256];
				sprintf_s(msg, "Could not open file '%s' (error %d).", filename, GetLastError());
				OutputDebugStringA(msg);
				throw FileNotFoundException(msg);
			}

			// Save the full pathename
			GetFullPathNameA(filename,  MAX_PATH, _fullPathName, NULL);

			// Read the header
			try
			{
				ReadHeader();
			}
			catch (OckFileException &e)
			{
				// Clean up and rethrow
				CloseHandle(_hFile);
				throw e;
			}
	

			 // File mapping
			_hFileMap = CreateFileMappingA(
				_hFile,						// File handle
				NULL,						// Security attributes
				PAGE_READONLY,				// page protection of the file mapping object.
				0,							// The high-order DWORD of the maximum size of the file mapping object
				0,							// The low-order DWORD of the maximum size of the file mapping object.
				NULL						// The name of the file mapping object
				);
			assert(_hFileMap != NULL);

			_pMapBase = (UINT8*)MapViewOfFile(
				_hFileMap,					// A handle to a file mapping object.
				FILE_MAP_READ,				// The type of access to a file mapping object
				0,							// A high-order DWORD of the file offset where the view begins
				0,							// A low-order DWORD of the file offset where the view is to begin. 
				0							// The number of bytes of a file mapping to map to the view
				);
			assert(_pMapBase != NULL);

		}


		OckFileSource::~OckFileSource()
		{
			UnmapViewOfFile(_pMapBase);
			CloseHandle(_hFileMap);
			CloseHandle(_hFile);

			SAFE_DELETE(_pAxis1Info);
			SAFE_DELETE(_pAxis2Info);
			SAFE_DELETE(_pAxis3Info);
		}


		INT16 *OckFileSource::GetFramePtrChanA(int frame)
		{
			if (frame < 0 || frame >= _frameCount)
			{
				// Argument out of range
				char msg[256];
				sprintf_s(msg, "Value for frame (%d) is out of range (0<=frame<%d).", frame, _frameCount);
				OutputDebugStringA(msg);
				throw ArgumentOutOfRangeException(msg);
			}

			LONGLONG offset = frame*((LONGLONG)_bytesPerFrame);
			INT16* pFrame = (INT16*)(_pMapBase + _headerLength + offset);

			return pFrame;
		}

		INT16 *OckFileSource::GetFramePtrChanB(int frame)
		{
			if (frame < 0 || frame >= _frameCount)
			{
				// Argument out of range
				char msg[256];
				sprintf_s(msg, "Value for frame (%d) is out of range (0<=frame<%d).", frame, _frameCount);
				OutputDebugStringA(msg);
				throw ArgumentOutOfRangeException(msg);
			}

			LONGLONG offset = frame*((LONGLONG)_bytesPerFrame);
			INT16* pFrame = (INT16*)(_pMapBase + _headerLength + offset + _bytesPerFrameChanA);

			return pFrame;
		}

		void OckFileSource::CopyChannelAFrameToBuffer(int frame, INT16 *buf)
		{
			if (frame < 0 || frame >= _frameCount)
			{
				// Argument out of range
				char msg[256];
				sprintf_s(msg, "Value for frame (%d) is out of range (0<=frame<%d).", frame, _frameCount);
				OutputDebugStringA(msg);
				throw ArgumentOutOfRangeException(msg);
			}

			LARGE_INTEGER offset;
			offset.QuadPart = _headerLength + frame*((LONGLONG)_bytesPerFrame);
			SetFilePointerEx(_hFile, offset, NULL, FILE_BEGIN);

			DWORD numberOfBytesRead = 0;
			ReadFile(_hFile, buf, _bytesPerFrameChanA, &numberOfBytesRead, NULL);
			assert(numberOfBytesRead == _bytesPerFrameChanA);

		}

		void OckFileSource::CopyFluorescenceDataToBuffer(INT16 *pDst, int dstPitch, int width, int height)
		{
			assert(width <= _aLineRate);
			assert(height <= _frameCount);
			
			Ipp16s *pSrc = GetFramePtrChanB(0);
		
			for (int y = 0; y < height; y++)
			{
				Ipp16s *pRowSrc = pSrc;
				for(int x = 0; x < width; x++)
				{
					ippsMean_16s_Sfs(pRowSrc, _kSpaceSamplesPerALine, &pDst[x], 0);
					pRowSrc += _bytesPerKSpaceALine >> 1;
				}


				pDst += dstPitch >> 1;
				pSrc += _bytesPerFrame >> 1;
			}


		}

		void OckFileSource::ReadHeader()
		{
			BOOL bSuccess;


			// Find length of file
			LARGE_INTEGER fileSize;
			GetFileSizeEx(_hFile, &fileSize);

			DWORD numberOfBytesRead;

			WA(ReadFile(_hFile, &_aLinesPerFrame, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_kSpaceSamplesPerALine, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_headerLength, sizeof(int), &numberOfBytesRead, NULL));

			// Application version
			char appVersionString[16];
			WA(ReadFile(_hFile, &appVersionString, sizeof(appVersionString), &numberOfBytesRead, NULL));
			sscanf_s(appVersionString, "%d.%d.%d", &_appVersion[0], &_appVersion[1], &_appVersion[3]);
			_appVersion[2] = 0;

			// Channel select (mask)
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.ChannelSelect, sizeof(int), &numberOfBytesRead, NULL));

			// Time stamp
			SetFilePointer(_hFile, 40, NULL, FILE_BEGIN);
			char timeStampString[14];
			WA(ReadFile(_hFile, timeStampString, sizeof(timeStampString), &numberOfBytesRead, NULL));	// yyyymmddhhmmss
			memset(&_timeStamp, 0, sizeof(_timeStamp));
			sscanf_s(timeStampString, "%4hu%2hu%2hu%2hu%2hu%2hu",
				&_timeStamp.wYear,
				&_timeStamp.wMonth,
				&_timeStamp.wDay,
				&_timeStamp.wHour,
				&_timeStamp.wMinute,
				&_timeStamp.wSecond);


			// Pullback settings
			SetFilePointer(_hFile, 128, NULL, FILE_BEGIN);
			int catheterSerialNumber;
			WA(ReadFile(_hFile, &catheterSerialNumber, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_frameRate, sizeof(double), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_pullbackRate, sizeof(double), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_pullbackLength, sizeof(double), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_pullbackStartPosition, sizeof(double), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_pullbackAcceleration, sizeof(double), &numberOfBytesRead, NULL));

			// Data acquiistion settings
			SetFilePointer(_hFile, 256, NULL, FILE_BEGIN);
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.ClockSource, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.SampleFrequency, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.ClockEdge, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.ChannelACoupling, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.ChannelARange, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.ChannelBCoupling, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.ChannelBRange, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.AuxIoMode, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.AuxOther, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.TriggerMode, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.SelfTriggerDelay, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.TriggerSlope, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.TriggerCoupling, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.TriggerLevel, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.TriggerRange, sizeof(int), &numberOfBytesRead, NULL));
			WA(ReadFile(_hFile, &_dataAcqusitionSettings.TriggerSource, sizeof(int), &numberOfBytesRead, NULL));

			// Application seetings (only get the ones we need)
			//memcpy(dst + 384, &AxialRange, sizeof(int));
			//memcpy(dst + 388, &Algorithm, sizeof(int));
			//memcpy(dst + 392, &ProcFunc, sizeof(int));
			//memcpy(dst + 396, &cMap, sizeof(int));
			//memcpy(dst + 400, &CartSize, sizeof(int));
			//memcpy(dst + 404, &Interpolation, sizeof(int));
			//memcpy(dst + 408, &OCTOffset, sizeof(int));
			//memcpy(dst + 412, &Image_vmin, sizeof(int));
			//memcpy(dst + 416, &Image_vmax, sizeof(int));
	
			// Immersion mediun
			// Air = 0, Water = 1, Saline = 2, Tissue = 3
			SetFilePointer(_hFile, 420, NULL, FILE_BEGIN);
			WA(ReadFile(_hFile, &_immersionMedium, sizeof(int), &numberOfBytesRead, NULL));

			// Need windowMask to determine source of channel B data
			// PolarWindow = 1, CartWindow = 2, DopWindow = 4, AFWindow =8
			int windowMask;
			WA(ReadFile(_hFile, &windowMask, sizeof(int), &numberOfBytesRead, NULL));


			//memcpy(dst + 428, &PlaybackRate, sizeof(int));
			//memcpy(dst + 432, &EnableOverlay, sizeof(int));
			//memcpy(dst + 436, &RemoveMotion, sizeof(int));
			//memcpy(dst + 440, &HistogramWidth, sizeof(int));
			//memcpy(dst + 444, &EnsembleLength, sizeof(int));
			//memcpy(dst + 448, &ROI_Low, sizeof(int));
			//memcpy(dst + 452, &ROI_High, sizeof(int));
			//memcpy(dst + 456, &structualThres, sizeof(int));
			//memcpy(dst + 460, &dopplerThres, sizeof(int));
			//memcpy(dst + 464, &AF_vmin, sizeof(int));
			//memcpy(dst + 468, &AF_vmax, sizeof(int));
			//memcpy(dst + 472, &ISAFDCCoup, sizeof(int));
			//memcpy(dst + 476, &chanAFRange, sizeof(int));
			//memcpy(dst + 480, &AFOffset, sizeof(int));
			//memcpy(dst + 484, &AFWidth, sizeof(int));

			// Probe name
			// ProbeName is version specific. Initial version of the file
			// included only the integer field 'catheterSerialNumber'.Later versions
			// included a string field 'catheterName' in addition to the
			// original field to allow for the inclusion of alphanumeric
			// caharcters.
			SetFilePointer(_hFile, 488, NULL, FILE_BEGIN);
			char catheterName[64];
			WA(ReadFile(_hFile, &catheterName, sizeof(catheterName), &numberOfBytesRead, NULL));
			if (strlen(catheterName) == 0)
			{
				// Build name from catheterSerialNumber
				sprintf_s(catheterName, "Catheter SN %d", catheterSerialNumber);
			}
			strcpy_s(_catheterName, catheterName);
	
			// AF Gain and ByteCount are version dependent
			// These fields are valid only if version>4
			if (_appVersion[0] > 4)
			{
				WA(ReadFile(_hFile, &_pmtGain, sizeof(double), &numberOfBytesRead, NULL));
				WA(ReadFile(_hFile, &_afByteCount, sizeof(int), &numberOfBytesRead, NULL));
			}
			else
			{
				_pmtGain = 0;
				_afByteCount = 0;
			}

			// Comment
			SetFilePointer(_hFile, 1024, NULL, FILE_BEGIN);
			WA(ReadFile(_hFile, _comment, sizeof(_comment), &numberOfBytesRead, NULL));


			// CALCAULTED PROPERTIES

			_aLineRate = _frameRate*_aLinesPerFrame;

			// Channel A Data
			if (_dataAcqusitionSettings.ChannelSelect & 0x0001)
			{
				_bytesPerFrameChanA = 2 * _kSpaceSamplesPerALine * _aLinesPerFrame;
			}
			else
			{
				// No channel A data
				_bytesPerFrameChanA = 0;
			}

			// Channel B Data
			if (_dataAcqusitionSettings.ChannelSelect & 0x0002)
			{
				//Length of each frame
				if (_afByteCount == 0)
					_bytesPerFrameChanB = 2 * _kSpaceSamplesPerALine * _aLinesPerFrame;
				else
					_bytesPerFrameChanB = _afByteCount;

				// Deterime source of channel B data
				if (_bytesPerFrameChanB > 0)
				{
					if (windowMask & 0x0008)
					{
						// AF Data
						_afDataAvailable = true;
						_afSamplesPerALine = _bytesPerFrameChanB / (2 * _aLinesPerFrame);
						assert(_afSamplesPerALine == 1 || _afSamplesPerALine == _kSpaceSamplesPerALine);
					}
					else
					{
						// OCT Data
						_polarizationDataAvailable = true;
					}
				}
			}
			else
			{
				// No Channel B data
				_bytesPerFrameChanB = 0;
			}

			// Frame Count
			_bytesPerFrame = _bytesPerFrameChanA + _bytesPerFrameChanB;
			_frameCount = int((fileSize.QuadPart - _headerLength) / _bytesPerFrame);
			_bytesPerKSpaceALine = 2 * _kSpaceSamplesPerALine;

			// Check for unexpected file size
			int extraBytes = (fileSize.QuadPart  - _headerLength) % _bytesPerFrame;
			if (extraBytes > 0)
			{
				// Throw exception
				char msg[256];
				sprintf_s(msg, "Ock file '%s' has an unexpected length. File may be corrupt.", _fullPathName);
				OutputDebugStringA(msg);
				throw OckFileException(msg);
			}

			// Hard - coded parameters
			_laserALineRate = 50400;
			_refractiveIndex = 1.333;
			_catheterDiameter = 0.900;
			_referenceLevel = 4.0810e+06;			// max value of a dataset


			// RTZ Scanning Limits

			// ALine scan range[mm of air]
			_pAxis1Info = new AxisInfo(0.0, 5.074, AxisType::Length, AxisUnits::Milimeters);
			_alineOffset = _pAxis1Info->GetFirst();


			double deltaTheta = 2 * M_PI / _aLinesPerFrame;
			//_axis2Range[0] = -M_PI;
			//_axis2Range[1] = deltaTheta * (_aLinesPerFrame - 1) - M_PI;
			//_pAxis2Info = AxisRange(-M_PI, deltaTheta * (_aLinesPerFrame - 1) - M_PI);
			_pAxis2Info = new AxisInfo(-M_PI, M_PI, AxisType::Angle, AxisUnits::Radians);



			double deltaZ = _pullbackRate * _aLinesPerFrame / _aLineRate;
			//_axis3Range[0] = 0.0;
			//_axis3Range[1] = deltaZ * (_frameCount - 1);
			//_pAxis3Info = AxisRange(0.0, deltaZ * (_frameCount - 1));
			_pAxis3Info = new AxisInfo(0, deltaZ * _frameCount, AxisType::Length, AxisUnits::Milimeters);

			return;

		WinApiError:

			// Read error
			assert(0);


	
		}

	} // namespace proc
} // namespace oct