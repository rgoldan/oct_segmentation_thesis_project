#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include "DataResource2D.h"
#include "ConstantBufferResource.h"
#include "PixelInfoResource.h"
#include "EventArgs.h"
#include "Definitions.h"

#include <exception>


namespace oct {
	namespace proc {

		// Forwatd defintion for Processor
		class Processor;
		class GpuProcessor;
		struct ColorChangedEventArgs;

		//[event_source(native)]
		class RenderTarget
		{

		public:

			RenderTarget(Processor* const pProcessor, Orientation orientation);
			virtual ~RenderTarget();
			
			virtual void Render(void * pResource, bool isNewSurface) = 0;
			virtual ACQUISTIONPOINT ConvertScreenCoordinatesToAcquistionIndices(int x, int y) = 0;
			
			virtual void OnPositionChanged(ACQUISTIONPOINT oldPoint, ACQUISTIONPOINT newPoint, RenderTarget* pInitiator) = 0;
			virtual void OnLevelsChanged(ColorChangedEventArgs args) = 0;
			virtual void OnColormapChanged(ColorChangedEventArgs args) = 0;


			// Catheter position
			virtual CATHETERINFO GetCatheterInfo() = 0;
			virtual void OnCatheterEdgePositionChanged(PositionChangedEventArgs args) = 0;
			virtual void NotifyCatheterEdgePositionChanged(double position , RenderTarget* pInitiator) = 0;
			virtual void SetProposedCatheterEdgePosition(double normalizedPosition) = 0;
			virtual double GetProposedCatheterEdgePosition() = 0;
			virtual void CommitProposedCatheterEdgePosition() = 0;

			virtual double GetNormalizedOffsetPosition();
			virtual double GetNormalizedCatheterEdgePosition();
			virtual void SetNormalizedCatheterEdgePosition(double value);

			

			
			// Levels
			float GetWhiteLevel();
			void SetWhiteLevel(float value);
			float GetBlackLevel();
			void SetBlackLevel(float value);
			float GetBlackLevelDefault();
			float GetWhiteLevelDefault();
			void ResetLevels();
			
			// Colormap
			void SetColormap(ColormapType type);
			ColormapType GetColormap();
			ColormapType GetColormapDefault();
			void ResetColormap();


			PIXELINFO GetPixelInfoFromShader(int x, int y);

			SIZE GetScaledSize(double scale);
			int GetFullScaleWidth();
			int GetFullScaleHeight();
			double GetAspectRatio();

			int GetWidth()
			{
				return (int)_width;
			}
			int GetHeight()
			{
				return (int)_height;
			}

			ImageModailityType GetImageModality()
			{
				return _imageModaility;
			}

			PixelInfo GetPixelInfo(int x, int y);


			int GetPixelValueFromRenderTarget(int x, int y);
			POINTD ConvertScreenCoordinatesToAcquisitionCoordinates(int x, int y);

			ConstantBufferResource* GetConstantBuffer() {
				return _pConstantBuffer;
			};

		protected:

			double ScreenXToNormalizedScreenX(int value)
			{
				value = CLAMP(value, 0, (int)_width - 1);				// Clip value to 0<=x<_width
				double normValue = double(value) / double(_width);		// 0<=x<1
				return normValue;
			}

			double ScreenYToNormalizedScreenY(int value)
			{
				value = CLAMP(value, 0, (int)_height - 1);				// Clip value to 0<=y<_width
				double normValue = double(value) / double(_height);		// 0<=y<1
				return normValue;
			}

			int ScreenXToAcquistionIndexX(int value)
			{
				double norm = ScreenXToNormalizedScreenX(value);
				return _aquitionIndexRangeX[0] + int(norm*(_aquitionIndexRangeX[1] - _aquitionIndexRangeX[0]));
			}

			int ScreenYToAcquistionIndexY(int value)
			{
				double norm = ScreenYToNormalizedScreenY(value);
				return _aquitionIndexRangeY[0] + int(norm*(_aquitionIndexRangeY[1] - _aquitionIndexRangeY[0]));
			}

			double AcquitionIndexXToNormalizedScreenX(int value)
			{
				double numerator = double(value - _aquitionIndexRangeX[0]);
				double denominator = double(_aquitionIndexRangeX[1] - _aquitionIndexRangeX[0]);
				return numerator / denominator;
			}

			double AcquitionIndexYToNormalizedScreenY(int value)
			{
				double numerator = double(value - _aquitionIndexRangeY[0]);
				double denominator = double(_aquitionIndexRangeY[1] - _aquitionIndexRangeY[0]);
				return numerator / denominator;
			}

			int AcquitionIndexXToScreenX(int value)
			{
				return NormalizedScreenXToScreenX(AcquitionIndexXToNormalizedScreenX(value));
			}

			int AcquitionIndexYToScreenY(int value)
			{
				return NormalizedScreenYToScreenY(AcquitionIndexYToNormalizedScreenY(value));
			}

			int NormalizedScreenXToScreenX(double value)
			{
				return int(value*_width);
			}

			int NormalizedScreenYToScreenY(double value)
			{
				return int(value*_height);
			}

			void InitRenderTarget(void * pResource);
			
			void NotifyLevelsChanged();
			void NotifyColormapChanged();




		protected:

			DataResource2D*			_pSrcData = NULL;

			Processor* const		_pProcessor = NULL;
			

			ConstantBufferResource* _pConstantBuffer = NULL;
			
			ID3D11RenderTargetView*	_pRenderTargetView = NULL;

			Orientation				_orientation = Orientation::Default;
			ImageModailityType		_imageModaility = ImageModailityType::Undefined;

			UINT					_width = 0;
			UINT					_height = 0;

			UINT					_fullScaleWidth = 0;
			UINT					_fullScaleHeight = 0;

			D3D11_VIEWPORT			_viewPort = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

			AxisInfo*				_pAxisXInfo = NULL;
			AxisInfo*				_pAxisYInfo = NULL;
			
			int						_aquitionIndexRangeX[2] = { 0,0 };
			int						_aquitionIndexRangeY[2] = { 0,0 };

		};

	} // namespace proc
} // namespace oct