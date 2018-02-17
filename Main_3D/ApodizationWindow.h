#pragma once

namespace oct {
	namespace proc {

		enum class ApodizationWindowType
		{
			None = 0,
			Bartlett = 1,
			Blackman = 2,
			Hamming = 3,
			Hann = 4,
			Kaiser = 5,
		};

		class ApodizationWindow
		{
			//friend class OctProcessor;

		public:
			ApodizationWindow(int length);
			ApodizationWindow(int length, ApodizationWindowType type);
			~ApodizationWindow();

		private:
			ApodizationWindowType	 _type;
			float					*_pData;
			float					_alpha;
			int						_length;

		public:
			ApodizationWindowType GetType() {
				return _type;
			}
			void SetType(ApodizationWindowType type);
			int GetLength() {
				return _length;
			}
			float *GetDataPtr() {
				return _pData;
			}

		private:
			void Build();

			};

	} // namespace proc
} // namespace oct