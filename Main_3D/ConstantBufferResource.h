#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "ColormapArrayResource.h"

namespace oct {
	namespace proc {


		// Structure must macth shader stucture
		struct CONSTANTBUFFERDATA
		{
			// LEVELS
			float black;
			float white;
			float contrast;

			// Colormap
			int colormap;

			// POLAR INTERPOLATION
			float rho1;
			float rho2;
			float deltaRho;
			
			// Pad to multiple of 16 bytes
			float reserved[1];		

		};
		static_assert(sizeof(CONSTANTBUFFERDATA)%16 == 0, "Length of CONSTANTBUFFERDATA must be a multiple of 16 bytes.");



		class ConstantBufferResource
		{
		public:
			ConstantBufferResource(ID3D11Device *pDevice, float black, float white, ColormapType colormap);
			~ConstantBufferResource();


			ID3D11Buffer* GetBuffer()
			{
				return _pBuffer;
			}


			// Levels
			void SetBlackLevel(float value);
			float GetBlackLevel() { return _data.black; }
			float GetBlackLevelDefault() { return _defaultData.black; }
			void SetWhiteLevel(float value);
			float GetWhiteLevel() { return _data.white; }
			float GetWhiteLevelDefault() { return _defaultData.white; }
			void ResetLevels();

			// Colormap
			ColormapType GetColormap() { return static_cast<ColormapType>(_data.colormap); }
			ColormapType GetColormapDefault() { return static_cast<ColormapType>(_defaultData.colormap); }
			void SetColormap(ColormapType value);
			void ResetColormap();


			// Polar interpolation
			void SetRho1(float value);
			float GetRho1() { return _data.rho1; }
			float GetRho1Default() { return _defaultData.rho1; }





			void CommitChangesToGpu(ID3D11DeviceContext* pImmediateContext);

			BOOL IsCommitToGpuRequired() { return _commitRequired;  }

		protected:

			HRESULT CopyToGpu(ID3D11DeviceContext* pImmediateContext);

		protected:

			ID3D11Buffer*				_pBuffer = NULL;

			CONSTANTBUFFERDATA			_data;
			CONSTANTBUFFERDATA			_defaultData;

			BOOL						_commitRequired = FALSE;

		};

	} // namespace proc
} // namespace oct
