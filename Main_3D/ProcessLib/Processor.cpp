#include "stdafx.h"
#include "Processor.h"

#include "VertexShader.h"
#include "PixelShader.h"

using namespace DirectX;
using namespace DirectX::PackedVector;


struct SimpleVertex
{
	XMFLOAT2 Pos;
	XMFLOAT2 Tex;
};

namespace oct {
	namespace proc {

		Processor::Processor()
		{
			_pCpuProcessor = new CpuProcessor();
			_pGpuProcessor = new GpuProcessor();
		}


		Processor::~Processor()
		{
			SAFE_DELETE(_pCpuProcessor);
			SAFE_DELETE(_pGpuProcessor);

			ippiFree(_pBuf);
		}
		
		void Processor::SetSource(OckFileSource *pSource)
		{
			int K1 = pSource->GetKSpaceLength();
			_pCpuProcessor->SetKSpaceSamplesPerALine(K1);
			int X1Length = _pCpuProcessor->GetSamplesPerALine();
			int X2Length = pSource->GetAxis2Length();
			int X3Length = pSource->GetAxis3Length();
			
			_pBuf = ippiMalloc_32f_C1(K1, X2Length, &_bufPitch);

			//_pGpuProcessor->SetLevels(-70.0, 0.0);
			_pCpuProcessor->SetReferenceLevel(pSource->GetReferenceLevel());
			
			// Sections
			_pGpuProcessor->InitDataResources(X1Length, X2Length, X3Length);

			_pSource = pSource;

			SetX1(0);
			SetX2(0);
			SetX3(0);

			// Load AFI
			_pGpuProcessor->SetAfiData(pSource);


		}

		void Processor::SetX1(int value, RenderTarget* pInitiator)
		{
			if (value == _currentPoint.X1) return;

			if (value<0 || value >= GetLengthX1())
			{
				// Argument out of range
				char msg[256];
				sprintf_s(msg, "Value for X1 (%d) is out of range (0<=X1<%d).", value, GetLengthX1());
				OutputDebugStringA(msg);
				throw ArgumentOutOfRangeException(msg);
			}

			ACQUISTIONPOINT oldPoint = _currentPoint;
			_currentPoint.X1 = value;

			_pGpuProcessor->NotifyPositionChanged(oldPoint, _currentPoint, pInitiator);
		}

		void Processor::SetX2(int value, RenderTarget* pInitiator)
		{
			if (value == _currentPoint.X2) return;

			if (value<0 || value >= GetLengthX2())
			{
				// Argument out of range
				char msg[256];
				sprintf_s(msg, "Value for X2 (%d) is out of range (0<=X2<%d).", value, GetLengthX2());
				OutputDebugStringA(msg);
				throw ArgumentOutOfRangeException(msg);
			}

			ACQUISTIONPOINT oldPoint = _currentPoint;
			_currentPoint.X2 = value;

			_pGpuProcessor->NotifyPositionChanged(oldPoint, _currentPoint, pInitiator);
		}

		void Processor::SetX3(int value, RenderTarget* pInitiator)
		{
			if (value == _currentPoint.X3) return;

			int N3 = _pSource->GetAxis3Length();

			if (value<0 || value>=N3)
			{
				// Argument out of range
				char msg[256];
				sprintf_s(msg, "Value for X3 (%d) is out of range (0<=X3<%d).", value, N3);
				OutputDebugStringA(msg);
				throw ArgumentOutOfRangeException(msg);
			}

			ACQUISTIONPOINT oldPoint = _currentPoint;
			_currentPoint.X3 = value;
			
			int K1 = _pSource->GetKSpaceLength();
			int N2 = _pSource->GetAxis2Length();
			int bytePitch2 = _pSource->GetAxis2BytePitch();

			// Process B-Frame
			_pCpuProcessor->ProcessALines(_pSource->GetFramePtrChanA(_currentPoint.X3), bytePitch2, K1, N2, _pBuf, _bufPitch);
			_pGpuProcessor->CopyToX2X1Section(_pBuf, _bufPitch);


			_pGpuProcessor->NotifyPositionChanged(oldPoint, _currentPoint, pInitiator);
		}


		void Processor::SetProposedCatheterEdgePosition(double position, RenderTarget* pInitiator)
		{
			// Save the value
			_proposedCatheterEdgePosition = position;
			
			// Propagate value to all render targets
			_pGpuProcessor->NotifyCatheterEdgePositionChanged(position, pInitiator);
		}
	

	} // namespace proc
} // namespace oct