#pragma once
#include "Utility.h"	


namespace oct {
	namespace proc {

		enum AxisType
		{
			DefaultType = 0,
			Length = 1,
			Time = 2,
			Angle = 3,
		};

		enum AxisUnits
		{
			DefaultUnits = 0,
			Milimeters = 1,
			Micrometers = 2,
			Radians = 3,
		};

		class AxisInfo
		{
		public:
			AxisInfo() {}
			AxisInfo(double A, double B) : _A(A), _B(B) {}
			AxisInfo(double A, double B, AxisType type, AxisUnits units) : _A(A), _B(B), _type(type), _units(units) {}
		private:
			double _A = 0.0;	// inclusive
			double _B = 0.0;	// exclusive
			AxisType _type = AxisType::DefaultType;
			AxisUnits _units = AxisUnits::DefaultUnits;
		public:
			double GetFirst() { return _A; }
			double GetLast() { return _B; }
			double GetDelta() { return _B - _A; }
			AxisUnits GetUnits() { return _units;  }
			AxisType GetType() { return _type; }

			void SwapFirstLast() { double temp = _A; _B = _A; _A = temp; };
		};

		class DataAcqusitionSettings
		{
		public:
			DataAcqusitionSettings();
			~DataAcqusitionSettings();

		public:

			int ChannelSelect;			// 1 = ChanA Only, 2 = ChanB Only, 3 = Both Chan Used
			int ClockSource;
			int SampleFrequency;
			int ClockEdge;				// Rising / Falling
			int ChannelACoupling;		// 1 = DC Coup
			int ChannelARange;
			int ChannelBCoupling;		// 1 = DC Coup
			int ChannelBRange;
			int AuxIoMode;
			int AuxOther;
			int TriggerMode;
			int SelfTriggerDelay;
			int TriggerSlope;			//	1 = trigger Slope Positive
			int TriggerCoupling;		// 1 = trigger DC Coup
			int TriggerLevel ;
			int TriggerRange;
			int TriggerSource;

		};


		class OckFileSource
		{

		public:
			OckFileSource(const char *filename);
			~OckFileSource();

		private:

			HANDLE _hFile;
			HANDLE _hFileMap;
			char _fullPathName[MAX_PATH];

			UINT8 *_pMapBase;

			SYSTEMTIME _timeStamp;

			int _kSpaceSamplesPerALine;
			int	_aLinesPerFrame;
			int	_frameCount;

			float _referenceLevel;


			double _catheterDiameter;
			double _pullbackRate;          // mm/sec
			double _refractiveIndex;
			double _aLineRate;              // Hz
			double _laserALineRate;         // Hz
			double _frameRate;              // Hz

			AxisInfo *_pAxis1Info = NULL;          // ALine scan range in mm of air
			AxisInfo *_pAxis2Info = NULL;
			AxisInfo *_pAxis3Info = NULL;
			double	_alineOffset = 0.0;				// Position of aline offset in mm of air

			int _headerLength;
			int _appVersion[4];
			unsigned int _channelMask;		

			double _pullbackLength;
			double _pullbackStartPosition;
			double _pullbackAcceleration;

			DataAcqusitionSettings _dataAcqusitionSettings;

			char _comment[1024];
			char _catheterName[64];

			double _pmtGain;
			int _afByteCount;

			int _bytesPerFrame = 0;
			int _bytesPerFrameChanA = 0;
			int _bytesPerFrameChanB = 0;
			int _afSamplesPerALine = 0;
			int _bytesPerKSpaceALine = 0;

			bool _polarizationDataAvailable = false;
			bool _afDataAvailable = false;

			int	_immersionMedium;


	
		public:
			
			//int GetALinesPerFrame() { return _aLinesPerFrame; }

			void CopyChannelAFrameToBuffer(int frame, INT16 *buf);
			void CopyFluorescenceDataToBuffer(INT16 *buf, int stepBytes, int width, int height);
			
			INT16 *GetFramePtrChanA(int frame);
			INT16 *GetFramePtrChanB(int frame);

			// Axis 1 (ALine)
			int GetKSpaceLength() {
				return _kSpaceSamplesPerALine;		// k-space samples per ALine
			}			

			AxisInfo &GetAxis1Info() { return *_pAxis1Info; }

			// Axis 2
			int GetAxis2Length() { return _aLinesPerFrame; }
			int GetAxis2BytePitch() { return _bytesPerKSpaceALine; };			// Byte offset between adjacent ALines
			AxisInfo &GetAxis2Info() { return *_pAxis2Info; }


			//Axis 3
			int GetAxis3Length() { return _frameCount; }
			int GetAxis3BytePitch() { return _bytesPerFrame; };					// Byte offset between adjacent BScans
			AxisInfo &GetAxis3Info() { return *_pAxis3Info; }

			float GetReferenceLevel()
			{
				return _referenceLevel;
			}

			// ALineOffset
			// Value is not persisted in ock file
			double GetALineOffset() { return _alineOffset;  }
			double GetNormalizedALineOffset()
			{
				return (_alineOffset - _pAxis1Info->GetFirst()) / _pAxis1Info->GetDelta();
			}
			void SetALineOffset(double value) { _alineOffset = value; }

			double GetCatheterDiameter() { return _catheterDiameter;  }
			double GetCatheterEdgePosition() { return _alineOffset + _catheterDiameter/2.0; }
			void SetCatheterEdgePosition(double value) { _alineOffset = value - _catheterDiameter / 2.0; }


		private:
			void ReadHeader();

		};

	} // namespace proc
} // namespace oct