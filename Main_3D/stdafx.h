// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _USE_MATH_DEFINES 


// TODO: reference additional headers your program requires here
#include <windows.h>

#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>	
#include <functional> // exception

//#define _USE_MATH_DEFINES 
#include <cmath>

// IPP
#include "ipps.h"
#include "ippcc.h"
#include "ipp.h"
#include "ippi.h"
#include "ippcore.h"
#include "ippvm.h"

// WIC
#include "Wincodec.h"

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <omp.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

using namespace std;
using namespace cv;


struct point {
	float x;
	float y;
}points;

struct frame {
	vector<point> coords;
	vector<float> startPtRefs;
	vector<float> surfRefs;
	float frameRef;
}frames;