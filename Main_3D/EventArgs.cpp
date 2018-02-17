#include "stdafx.h"

#include "EventArgs.h"
#include "RenderTarget.h"

namespace oct {
	namespace proc {


		ColorChangedEventArgs::ColorChangedEventArgs(RenderTarget *pRenderTarget)
		{
			Black = pRenderTarget->GetBlackLevel();
			White = pRenderTarget->GetWhiteLevel();
			Colormap = pRenderTarget->GetColormap();
		}

	} // namespace proc
} // namespace oct