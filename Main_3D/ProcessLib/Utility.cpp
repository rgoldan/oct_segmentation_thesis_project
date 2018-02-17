#include "stdafx.h"

#include "Utility.h"

namespace oct {
	namespace proc {

		void GetErrorMessage(HRESULT hr, char *pszMsg, size_t length)
		{

			// Look for a message from the system
			// This dosn't work for DXGI_ERROR messages ....
			DWORD nLen = FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				hr,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				pszMsg,
				(DWORD)length,
				NULL
				);

			if (nLen)
			{
				// Remove trailing newline
				if (nLen > 1 && pszMsg[nLen - 1] == '\n')
				{
					pszMsg[nLen - 1] = 0;
					if (pszMsg[nLen - 2] == '\r')
					{
						pszMsg[nLen - 2] = 0;
					}
				}
			}
			else
			{
				// FormatMessageA failed to find message string
				switch (hr)
				{
				case DXGI_ERROR_UNSUPPORTED:
					sprintf_s(pszMsg, length, "The requested functionality is not supported by the device or the driver.");
					break;
				default:
					sprintf_s(pszMsg, length, "HRESULT = 0x%08X (No error message avaialble).", (UINT)hr);
				}
			}
		}


		void DXTraceEx(char *pSrcFile, int srcFileLine, HRESULT hr, char *x)
		{
			char errorText[256];
			GetErrorMessage(hr, errorText, sizeof(errorText));

			if (NULL != errorText)
			{
				char msg[1024];
				sprintf_s(msg, "D3D11 ERROR: '%s' returned '%s' (Line %d of '%s')\n", x, errorText, srcFileLine, pSrcFile);
				OutputDebugStringA(msg);
			}
		}

		void WATraceEx(char *pSrcFile, int srcFileLine, char *x)
		{
			char errorText[256];
			DWORD err = GetLastError();
			GetErrorMessage(err, errorText, sizeof(errorText));

			if (NULL != errorText)
			{
				char msg[1024];
				sprintf_s(msg, "WINAPI ERROR: '%s' returned '%s' (Line %d of '%s')\n", x, errorText, srcFileLine, pSrcFile);
				OutputDebugStringA(msg);
			}
		}


	} // namespace proc
} // namespace oct