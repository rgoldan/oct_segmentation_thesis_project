#pragma once


namespace oct {
	namespace proc {

		enum Orientation
		{
			Default = 0,
			FlipLeftRight = 1,
			FlipTopBottom = 2,
			Transpose = 4,
		};

		enum class ImageModailityType
		{
			Undefined = 0,
			OCT = 1,
			AFI = 2,
		};

		enum class CatheterEdgeProjection
		{
			Undefined = 0,
			Hidden,
			HorizontalLine,
			HorizontalLinePair,
			VerticalLine,
			VerticalLinePair,
			CenteredCircle,
		};

		inline Orientation operator|(Orientation a, Orientation b)
		{
			return static_cast<Orientation>(static_cast<int>(a) | static_cast<int>(b));
		}

		struct ACQUISTIONPOINT
		{
			ACQUISTIONPOINT() {};
			ACQUISTIONPOINT(int _X1, int _X2, int _X3) : X1(_X1), X2(_X2), X3(_X3) {};
			int X1 = 0;
			int X2 = 0;
			int X3 = 0;
		};


		struct POINTD
		{
			POINTD() {};
			POINTD(double _y, double _x) : x(_x), y(_y) {};
			double x = 0;
			double y = 0;
		};

		struct CATHETERINFO
		{
			CATHETERINFO() {};
			CATHETERINFO(double diameter, CatheterEdgeProjection projection) : 
				Diameter(diameter), EdgeProjection(projection) {};
			double Diameter = 0.0;	// mm
			CatheterEdgeProjection EdgeProjection = CatheterEdgeProjection::Undefined;
			int ScreenPosition = 0;			// pixels
			int ScreenDiameter = 0;			// pixels
		};



		struct PixelInfo
		{
		public:
			PixelInfo() {};
			PixelInfo(int _X, int _Y) : X(_X), Y(_Y) {};
			int					X = 0;
			int					Y = 0;
			ACQUISTIONPOINT		AcquisitionIndices;
			POINTD				AcquisitionCoordinates = { 0.0, 0.0 };
			int					argb = 0;
			float				value = 0.0f;
		};





	} // namespace proc
} // namespace oct