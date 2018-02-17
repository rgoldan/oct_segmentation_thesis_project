#pragma once
#include "stdafx.h"

class Segmentations
{
public:
	Segmentations(IppiSize roi, int dep, int numSurfs);
	~Segmentations();
	void addFramesN(Ipp8u* surfs, Ipp32f* endPointPtrs[], int* keepNums[],int frameNumber);
	void combineFrames();
	void writeToFile();


public:
	int numFrames;
	int width;
	int height;
	int numSurfs;
	FILE *pFile;
	vector<frame> segFrames;
	vector<point> coords;
	vector<float> startPtRefs;
	vector<float> surfaceRefs;
	vector<float> frameRefs;


};
