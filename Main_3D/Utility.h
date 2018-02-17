#pragma once

#ifndef CLAMP
#define CLAMP(_X,_A,_B)           (min(max(_A,_X),_B))
#endif

#ifndef SWAP
#define SWAP(_x, _y, _T) { _T tmp = _x; _x = _y; _y = tmp; }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)      { if (p) { delete (p); (p)=NULL; } }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)      { if (p) { delete [] (p); (p)=NULL; } }
#endif

#if defined(DEBUG) | defined(_DEBUG)
#define DXTRACE(_file_, _line_, _hr_, _x_) DXTraceEx(_file_, _line_, _hr_, _x_)
#define WATRACE(_file_, _line_, _x_) WATraceEx(_file_, _line_, _x_)
#else
#define DXTRACE(_file_, _line_, _hr_, _x_)
#define WATRACE(_file_, _line_, _x_)
#endif

// HRESULT ERROR CHECK
#ifndef HR
#define HR(_x_)												\
hr = (_x_);													\
if (FAILED(hr))												\
{															\
	DXTRACE(__FILE__, __LINE__, hr, #_x_);					\
	goto DXError;											\
}
#endif

// WINAPI ERROR CHECK
#ifndef WA
#define WA(_x_)												\
bSuccess = (_x_);											\
if (bSuccess==FALSE)										\
{															\
	WATRACE(__FILE__, __LINE__, #_x_);						\
	goto WinApiError;										\
}
#endif


namespace oct {
	namespace proc {

		void DXTraceEx(char *pSrcFile, int srcFileLine, HRESULT hr, char *x);
		void WATraceEx(char *pSrcFile, int srcFileLine, char *x);
		void GetErrorMessage(HRESULT hr, char *pszMsg, size_t length);



		class FileNotFoundException : public std::exception {
		public:
			explicit FileNotFoundException(const char *_Message) : std::exception(_Message) {}
		};

		class OckFileException : public std::exception {
		public:
			explicit OckFileException(const char *_Message) : std::exception(_Message) {}
		};

		
		class InvalidArgumentException : public std::exception {
		public:
			explicit InvalidArgumentException(const char *_Message) : std::exception(_Message) {}
		};

		class ArgumentOutOfRangeException : public std::exception {
		public:
			explicit ArgumentOutOfRangeException(const char *_Message) : std::exception(_Message) {}
		};


		class ProcessorException : public std::exception {
		public:
			explicit ProcessorException(const char *_Message) : std::exception(_Message), _hr(S_OK) {}
			explicit ProcessorException(const char *_Message, HRESULT hr) : std::exception(_Message), _hr(hr) {}
			HRESULT GetHResult() { return _hr; }
		private:
			HRESULT _hr;
		};

	} // namespace proc
} // namespace oct
