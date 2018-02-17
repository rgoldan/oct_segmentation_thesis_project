#include "stdafx.h"
#include "Segmentations.h"

// Class for saving Epithelium Surface and Basement Membrane segmentations
// Epithelium surface (or TS - top surface) saved as 1s
// Basement membrane saved as 2s

Segmentations::Segmentations(IppiSize roi, int dep, int nSurfs)
{
	numFrames =dep;
	width =roi.width;
	height = roi.height;
	numSurfs = nSurfs;
	float NF = (float)numFrames;
	float W = (float)width;
	float H = (float)height;
	std::chrono::time_point<std::chrono::system_clock> timeDate;
	timeDate = std::chrono::system_clock::now();
	std::time_t timeNdate = std::chrono::system_clock::to_time_t(timeDate);
	segFrames.resize(dep);
	frameRefs.resize(dep);

	char* timeD = std::ctime(&timeNdate);


	char outDate[100];
	strftime(&outDate[0], 100, "E:/segmentationsDirectory/%m%d%G_T%H%M%S.bin",std::localtime(&timeNdate));
	pFile = fopen(outDate, "wb");
	fwrite(&W, sizeof(float), 1, pFile);
	fwrite(&H, sizeof(float), 1, pFile);
	fwrite(&NF, sizeof(float), 1, pFile);
	
	
}



void Segmentations::addFramesN(Ipp8u* surfs, Ipp32f* endPointPtrs[], int* keepNums[],int frameNum) {
	//Can add segmentaion of any number of surfaces
	//Each surface will be composed of a single digit representing its edges (e.g. 1=TS, 2=BM, etc.)

	IppiSize roi = { width,height };
	Ipp8u* currentSurf = (Ipp8u*)ippsMalloc_8u(width*height);
	IppStatus status;
	vector<point> surfBuffer;
	int temp;
	int n;
	int m;
	int cc;
	Mat srf(Size(roi.width, roi.height), CV_8U, surfs);
	
	int kn;

	n = 0;
	for (int surf = 0; surf < numSurfs; surf++) {
		 m = 0;
		status = ippiCompareC_8u_C1R(surfs, width, surf + 1, currentSurf, width, roi, ippCmpEq); //extract desired surface from image 
		status = ippiDivC_8u_C1IRSfs(255, currentSurf, width, roi, 0);
		kn = keepNums[surf][0];

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				temp = currentSurf[j*width + i];

				if (temp == 1) {
					surfBuffer.resize(n + 1);
					surfBuffer[n].x = i;
					surfBuffer[n].y = j;

					cc = size(segFrames[frameNum].coords);

					segFrames[frameNum].coords.resize(cc + 1);
					segFrames[frameNum].coords[cc].x = i;
					segFrames[frameNum].coords[cc].y = j;
					n++;
				}
			}
		}
		
		float q = size(segFrames[frameNum].startPtRefs);
		segFrames[frameNum].surfRefs.push_back(q);
		
		for (int ii = 0; ii < n; ii++) {
			for (int jj = 0; jj < kn; jj++) {
				if (surfBuffer[ii].x == endPointPtrs[surf][jj * 4] && surfBuffer[ii].y == endPointPtrs[surf][jj * 4 + 1]) {
					segFrames[frameNum].startPtRefs.push_back(ii);
				}
			}
		}
	}
	float r = size(segFrames[frameNum].surfRefs);
	segFrames[frameNum].frameRef = r - 1;

	srf.release();
	ippsFree(currentSurf);
}

void Segmentations::writeToFile() {
	float cCount = (float)size(coords);
	float spCount = (float)size(startPtRefs);
	float srfCount = (float)size(surfaceRefs);
	float frCount = (float)size(frameRefs);
	fwrite(&cCount, sizeof(float), 1, pFile);
	fwrite(&coords[0], sizeof(points), cCount, pFile);
	fwrite(&spCount, sizeof(float), 1, pFile);
	fwrite(&startPtRefs[0], sizeof(float), spCount, pFile);
	fwrite(&srfCount, sizeof(float), 1, pFile);
	fwrite(&surfaceRefs[0], sizeof(float),srfCount , pFile);
	fwrite(&frCount, sizeof(float), 1, pFile);
	fwrite(&frameRefs[0], sizeof(float), frCount, pFile);
}

void Segmentations::combineFrames() {
	int csize, csizeI;
	int spSize, spSizeI;
	int srfSize, srfSizeI;
	int frSize, frSizeI;
	float current;
	for (int i = 0; i < numFrames; i++) {
		csizeI = size(segFrames[i].coords);
		csize = size(coords);
		coords.insert(coords.end(), segFrames[i].coords.begin(), segFrames[i].coords.end());

		spSize = size(startPtRefs);
		spSizeI = size(segFrames[i].startPtRefs);
		if (csize == 0) {
			csize = 0;
		}
		
		for (int m = 0; m < spSizeI; m++) {
			current = segFrames[i].startPtRefs[m];
			segFrames[i].startPtRefs[m] = current + csize;
		}
		startPtRefs.insert(startPtRefs.end(), segFrames[i].startPtRefs.begin(), segFrames[i].startPtRefs.end());
		
		srfSize = size(surfaceRefs);
		srfSizeI = size(segFrames[i].surfRefs);
		

		for (int m = 0; m < srfSizeI; m++) {
			current = segFrames[i].surfRefs[m];
			segFrames[i].surfRefs[m] = current + spSize;
		}

		surfaceRefs.insert(surfaceRefs.end(), segFrames[i].surfRefs.begin(), segFrames[i].surfRefs.end());
		frSize = size(frameRefs);
		frameRefs[i]=segFrames[i].frameRef+srfSize-1;

	}
}

Segmentations::~Segmentations()
{
	fclose(pFile);
	
};

