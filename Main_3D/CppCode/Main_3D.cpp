// Main_3D.cpp : Defines the entry point for the console application.
//
// Author: Ryan Goldan

#include "stdafx.h"
#include "Segmentations.h"



#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#include "../ProcessLib/OckFileSource.h"
#include "../ProcessLib/Processor.h"

using namespace std;
using namespace cv;


struct topSurfSpec {
	IppiSize TSroi;
	IppiPoint TSstart;
	int bottomCut;
}topSurfSpecs;



topSurfSpec tissueMask(Ipp8u* srcImg, IppiSize roi,int topcut);
Ipp8u* morphCloseAndOpen(Ipp8u* srcImg, IppiSize roi);
Ipp8u* canny3(Mat srcImg, double PBspeed, double SpinRate);
Ipp8u* selectEpith(Ipp32f* srcImg, IppiSize srcRoi, IppiPoint TSstart, IppiSize TScropROI, Ipp8u* edges, double PBspeed, double SpinRate,Ipp32f* endPoints,int* keepNumTS);
Ipp8u* selectBM(Ipp32f* srcImg, IppiSize srcRoi, IppiPoint TSstart, IppiSize BMcropROI, Ipp8u* edges, double PBspeed, double SpinRate, Ipp32f* endPoints, int* keepNumBM);
void bwlookup2(Ipp8u* inIpp, Ipp8u* outIpp, const cv::Mat & lut, int bordertype, cv::Scalar px, IppiSize roi);
Ipp32f* GaussSmooth2(Ipp32f* srcImg, IppiSize roi);
void mainFn(Ipp8u* ippImgPtr2, IppiSize roi, Ipp32f* endPointsTS, Ipp32f* endPointsBM, Ipp8u* BMsurfs, Ipp8u* TSsurfs, int topcut, int frame, int* keepNumTS, int* keepNumBM);
void avgLocs(int *locs,int indx, int dep);
void findNonZero2(Ipp8u* endPoints, Ipp32f* nonZeroCoords, IppiSize roi, int* numPoints);
void enfaceCutoffs(Ipp32f* enface, IppiSize roiEnface, Ipp8u* enfaceMask, int* centerFrame);
void createEpithMap(Ipp8u* TS, Ipp8u* BM, Ipp32f* epithMap, IppiSize roi, IppiSize roiEnface, int currentFrame);

using namespace oct::proc;

int main(int argc, char** argv)
{

	if (argc != 2)
	{
		cout << " Usage: display_image ImageToLoadAndDisplay" << endl;
		return -1;
	}

	//Load data from OCK file
	OckFileSource *pSource;
	Processor *pProc = new Processor();
	GpuProcessor* pGpuProc = pProc->GetGpuProcessor();

	try
	{
		pSource = new OckFileSource("E:\\OCKfiles\\PB_000667.OCK");
	}
	catch (OckFileException &e)
	{
		printf(e.what());
		pSource = NULL;
		
		return 0;
	}

	//Set default rows to crop from on top and bottom
	int topCut = 50;
	int bottomCut = 500;
	IppStatus status;

	// Set the OCT source
	pProc->SetSource(pSource);
	DataResource2D* pDataResource = pProc->GetGpuProcessor()->GetX1X2Data();
	float* buf = static_cast<float*>(pDataResource->GetPointerToCpuBuffer());
	Ipp32f* bufIpp = (Ipp32f*)buf;
	int bufPitch = pDataResource->GetCpuBufferPitch();
	int x1 = pProc->GetLengthX1();
	int x2 = pProc->GetLengthX2();
	int x3 = pProc->GetLengthX3();

	// Load bscans here n=0:pProc->GetLengthX2()-1
	int n = 0;
	
	int wid2 = x1;
	int hei2 = x3;
	int dep2 = x2; 
	int sstep3;

	IppiSize roiPolar = { wid2,dep2 };
	IppiSize roiAz = { wid2,hei2 };
	Ipp32f **volPolar;
	volPolar = new Ipp32f*[hei2];

	//array of pointers, each pointing to a polar frame
	for (int n1 = 0; n1 < hei2; n1++) {

		pProc->SetX3(n1);
		volPolar[n1] = ippiMalloc_32f_C1(wid2, dep2, &sstep3);
		status = ippiCopy_32f_C1R(bufIpp, roiPolar.width * 4, volPolar[n1], roiPolar.width * 4, roiPolar);
		
	}

	delete pSource;
	delete pProc;

	IppiSize roiEnface = { dep2,hei2 };
	Ipp32f *enface = (Ipp32f*)ippsMalloc_32f(roiEnface.width*roiEnface.height);
	Ipp32f *epithMap = (Ipp32f*)ippsMalloc_32f(roiEnface.width*roiEnface.height);

	//create enface image by taking mean of each A-line
	Ipp64f meanAline;
	for (int i = 0; i < roiEnface.width; i++) {
		for (int j = 0; j < roiEnface.height; j++) {
			status = ippiMean_32f_C1R(&volPolar[j][i*roiPolar.width], roiPolar.width * 4, { roiPolar.width,1 }, &meanAline, ippAlgHintNone);
			status = ippiSet_32f_C1R((Ipp32f)meanAline, &enface[j*roiEnface.width + i], roiEnface.width * 4, { 1,1 });
		}
	}

	int centerFrame;
	Ipp8u* enfaceMask = (Ipp8u*)ippsMalloc_8u(roiEnface.width*roiEnface.height);
	enfaceCutoffs(enface, roiEnface,enfaceMask, &centerFrame); //obtain mask to isolate tissue in enface image
	Mat enfc(Size(roiEnface.width, roiEnface.height), CV_32F, enface);
	Mat enfcmsk(Size(roiEnface.width, roiEnface.height), CV_8U, enfaceMask);

	//get slice from middle of the tissue 
	Ipp32f* midSlice = (Ipp32f*)ippsMalloc_32f(roiAz.width*roiAz.height);

	for (int j = 0; j < hei2; j++) {

		//get A-line from polar and place in azimuthal frame
		status = ippiCopy_32f_C1R(&volPolar[j][centerFrame*roiPolar.width + topCut], roiPolar.width * 4, &midSlice[j*roiAz.width], roiAz.width * 4, { roiAz.width,1 });
	}
	Mat test(Size(roiAz.width, roiAz.height), CV_32F, midSlice);
	Ipp8u* midSlice8u = (Ipp8u*)ippsMalloc_8u(roiAz.width*roiAz.height);

	//Normalize pixel values to range [0,255]
	Ipp32f minPx, maxPx, fscale;
	status = ippiMin_32f_C1R(midSlice, roiAz.width * 4, roiAz, &minPx);
	status = ippiAddC_32f_C1IR(minPx*(-1.0), midSlice, roiAz.width * 4, roiAz);
	status = ippiMax_32f_C1R(midSlice, roiAz.width * 4, roiAz, &maxPx);
	fscale = maxPx / 255;
	status = ippiDivC_32f_C1IR(fscale, midSlice, roiAz.width * 4, roiAz);
	status = ippiConvert_32f8u_C1R(midSlice, roiAz.width * 4, midSlice8u, roiAz.width, roiAz, ippRndNear);

	//determine new croping region based on looking at the middle slice
	//likely need to modify this
	topSurfSpec cropSpec = tissueMask(midSlice8u, roiAz, topCut); //get coordinates for isolating tissue surface
	bottomCut = cropSpec.bottomCut;

	if (cropSpec.TSstart.x - 100 > 0) {
		topCut = cropSpec.TSstart.x;
	}

	if (bottomCut + 200 < roiAz.width) {
		roiAz = { bottomCut - topCut+200,hei2 };
	}
	
	ippsFree(midSlice);
	ippsFree(midSlice8u);

	Mat image;
	
	int maxThreads=omp_get_max_threads();
	int numCores = omp_get_num_procs();

	//triangular azimuthal smoothing kernel
	float azKern[] = { 0.027777777777778
		,0.055555555555556
		,0.083333333333333
		,0.111111111111111
		,0.138888888888889
		,0.166666666666667
		,0.138888888888889
		,0.111111111111111
		,0.083333333333333
		,0.055555555555556
		,0.027777777777778 };
	int divisor = 1000;

	
	IppiSize roiCrop = { roiAz.width - 50,roiAz.height };	
	Segmentations segs(roiAz, dep2,2); //initialize segmentation class object
	vector<vector<float>> epithLines;
	epithLines.resize(dep2);
	
	int e = 11;
	/*-------------------------Begin azimuthal frame segmentation------------------------*/
	int countNumFrames;
	int m,j,i,k;
	Ipp8u maxAline;
#pragma omp parallel for schedule(dynamic) num_threads(4) private(i) private(j) private(m) shared(countNumFrames) //firstprivate(locs) //shared(epithMap) shared(segs) //num_threads(maxThreads) //parallelize the for-loop 
	for (k =0; k < dep2; k++) {
	
		int locs[11];
		//check max value in row corresponding to the current az frame in the enface mask
		status = ippiMax_8u_C1R(&enfaceMask[k], roiEnface.width, { 1,roiEnface.height }, &maxAline);

		//if frame does not belong to tissue surface, skip it
		if (maxAline != 0) {
			avgLocs(&locs[0], k, dep2);
			cout << k << "\n";
			countNumFrames++;
			Ipp32f **volTemp;
			volTemp = new Ipp32f*[11];

			int keepNumTS, keepNumBM;
			int** keepNums = new int*[2];
			keepNums[0] = &keepNumTS;
			keepNums[1] = &keepNumBM;

			//initialize buffers
			Ipp32f* endPointsTS = (Ipp32f*)ippsMalloc_32f(roiAz.width*roiAz.height);
			Ipp32f* endPointsBM = (Ipp32f*)ippsMalloc_32f(roiAz.width*roiAz.height);
			Ipp8u* BMandTS = (Ipp8u*)ippsMalloc_8u(roiAz.width*roiAz.height);
			Ipp8u* edgesBM = (Ipp8u*)ippsMalloc_8u(roiAz.width*roiAz.height);
			Ipp8u* edgesTS = (Ipp8u*)ippsMalloc_8u(roiAz.width*roiAz.height);
			Ipp32f** endPointPtrs = new Ipp32f*[2];
			endPointPtrs[0] = &endPointsTS[0];
			endPointPtrs[1] = &endPointsBM[0];

			//create array of 11 pointers to the frames to be averaged
			//azimuthal frame averaging
			for (m = 0; m < 11; m++) {
				volTemp[m] = ippsMalloc_32f(roiAz.width*roiAz.height);
				//create azimuthal frames from a-lines in polar frames
				for (j = 0; j < hei2; j++) {
					status = ippiCopy_32f_C1R(&volPolar[j][locs[m] * roiPolar.width + topCut], roiPolar.width * 4, &volTemp[m][j*roiAz.width], roiAz.width * 4, { roiAz.width,1 });
				
				}
				status = ippiMulC_32f_C1IR(azKern[m], volTemp[m], roiAz.width * 4, roiAz); //multiply by kernel value
			}

			for (m = 1; m < 11; m++) {
				status = ippiAdd_32f_C1IR(volTemp[m], roiAz.width * 4, volTemp[0], roiAz.width * 4, roiAz); //sum the frames to obtain averaged image
			}

			//Normalize pixel values to range [0,255]
			Ipp32f minPix, maxPix, scaleF;
			status = ippiMin_32f_C1R(volTemp[0], roiAz.width * 4, roiAz, &minPix);
			status = ippiAddC_32f_C1IR(minPix*(-1.0), volTemp[0], roiAz.width * 4, roiAz);
			status = ippiMax_32f_C1R(volTemp[0], roiAz.width * 4, roiAz, &maxPix);
			scaleF = maxPix / 255;
			status = ippiDivC_32f_C1IR(scaleF, volTemp[0], roiAz.width * 4, roiAz);


			Ipp8u* inImg8u = (Ipp8u*)ippsMalloc_8u(roiAz.width*roiAz.height);
			status = ippiConvert_32f8u_C1R(volTemp[0], roiAz.width * 4, inImg8u, roiAz.width, roiAz, ippRndNear);
			Mat srcim(Size(roiAz.width, roiAz.height), CV_8U, inImg8u);

			/*------2D Segmentation function------*/
			mainFn(inImg8u, roiAz, endPointsTS, endPointsBM, edgesBM, edgesTS, topCut, k,&keepNumTS,&keepNumBM);
			
			Mat bothSurf(Size(roiAz.width, roiAz.height), CV_8U, BMandTS);
			Mat dstT;
			status = ippiMulC_8u_C1IRSfs(2, edgesBM, roiAz.width, roiAz, 0);  //Display top surface as 1s, and basement membrane as 2s. Everything else 0
			status = ippiAdd_8u_C1RSfs(edgesTS, roiAz.width, edgesBM, roiAz.width, BMandTS, roiAz.width, roiAz, 0);
			transpose(bothSurf, dstT);

			createEpithMap(edgesTS, edgesBM, epithMap, roiAz, roiEnface, k); //obtain epithelial thickness of the az frame. Add as a row of the epith map

			segs.addFramesN(BMandTS, endPointPtrs, keepNums, k); //add frame to segmentation class object
			ippsFree(inImg8u);
			dstT.release();

			//release all alocated memory remaining
			for (i = 0; i < 11; i++) {
				ippsFree(volTemp[i]);
			}
			delete[] volTemp;
			
			ippsFree(BMandTS);
			ippsFree(edgesBM);
			ippsFree(edgesTS);
			delete[] endPointPtrs;
			ippsFree(endPointsTS);
			ippsFree(endPointsBM);
			delete[] keepNums;
		}
	}
	
	
	
	segs.combineFrames();
	segs.writeToFile();
	segs.~Segmentations();
	
	Mat epith(Size(roiEnface.width, roiEnface.height), CV_32F, epithMap);

	Ipp32f* enfaceMask32 = (Ipp32f*)ippsMalloc_32f(roiEnface.width*roiEnface.height);
	status = ippiConvert_8u32f_C1R(enfaceMask, roiEnface.width, enfaceMask32, roiEnface.width*4, roiEnface);
	status = ippiMul_32f_C1IR(enfaceMask32, roiEnface.width * 4, epithMap, roiEnface.width * 4, roiEnface); //apply enface mask to epith map
	
	ippsFree(enface);
	ippsFree(enfaceMask);
	
	waitKey(0);                                          // Wait for a keystroke in the window
	return 0;
}

void mainFn(Ipp8u* ippImgPtr2, IppiSize roi, Ipp32f* endPointsTS, Ipp32f* endPointsBM, Ipp8u* BMsurfs,Ipp8u* TSsurfs, int topcut, int frame,int* keepNumTS,int* keepNumBM) {
	const int PBSpeed = 4;
	const int SpinRate = 100;
	
	IppiSize roi2 = { roi.width - topcut,roi.height };
	topSurfSpec TSspec = tissueMask(ippImgPtr2, roi,0); //get coordinates for isolating tissue surface
	int bottomCut = TSspec.bottomCut;
	Size ksize = { 45,31 }; //Double check this is the correct order. 45-> r-dimension and 31-> z-dimension
		
	double sigmaX = 8.2, sigmaY = 5.5; //smoothing kernel parameters
	int padX = ceil(sigmaX*2.6); //determine amount of padding needed when smoothing the image
	int padY = ceil(sigmaY*2.6);
	IppiSize roiPad = { roi.width + padX * 2,roi.height + padY * 2 }; //check this
													

	Ipp32f* ippIm32 = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	IppStatus status = ippiConvert_8u32f_C1R(ippImgPtr2, roi.width, ippIm32, roi.width * 4, roi); //convert to floating point

	/*-------------------Gaussian Smoothing---------------------*/
	Ipp32f* blurredIm = GaussSmooth2(ippIm32, roi); //apply Gaussian smoothing to source image
	Mat dst(Size(roi.width, roi.height), CV_32F, blurredIm);

	/*------------------------Canny--------------------------*/
	Ipp8u* edges = canny3(dst, PBSpeed, SpinRate); //perform Canny edge detection 
	
	Mat dstImg(Size(roi.width, roi.height), CV_8U, edges);
	Mat dstT;

	Ipp8u *edges2 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	Ipp8u *edges3 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);

	//make two copies of edges, since some in-place operations will change its values
	status = ippiCopy_8u_C1R(edges, roi.width, edges2, roi.width, roi);
	status = ippiCopy_8u_C1R(edges, roi.width, edges3, roi.width, roi);
	ippsFree(edges3);

	IppiSize cropRoi = TSspec.TSroi; 

	/*------------------Detect Top Surface-------------------*/

	Ipp8u* edgesTS = selectEpith(blurredIm, roi, TSspec.TSstart, cropRoi, edges, PBSpeed, SpinRate,endPointsTS, keepNumTS); //pass in the ROI which will only operate on the top surface region
	Ipp8u* edgesTS2 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);

	//Blackout the all of the edges above the detected top surface
	//these are usually the reflections from the sheaths of the OCT probe 
	//other image artifacts as well
	int indx;
	for (int row = 0; row < roi.height; row++) {
		for (int col = 0; col < roi.width; col++) {
			indx = row*roi.width + col; //position in pointed array of each pixel
			if (edgesTS[indx] == 255) {
				break;
			}
			edges2[indx] = 0;
		}
	}

	//Also blackout the top surface, since we don't want to detect it again as BM
 	status = ippiNot_8u_C1R(edgesTS, roi.width, edgesTS2, roi.width, roi); //invert the TS edge image
	status = ippiMul_8u_C1IRSfs(edgesTS2, roi.width, edges2, roi.width, roi, 0); //multiply by the image with everything above TS blacked out
																				 //this leaves just the edges detected bellow the TS
	Ipp8u maxVal;
	status = ippiMax_8u_C1R(edges2, roi.width, roi, &maxVal);

	//If there are no edges detected below top surface, image will be all 0s and cause error
	//put 2 white pixels in corner so that it doesn't crash later on
	if (maxVal == 0) {
		status = ippiSet_8u_C1R(255, &edges2[roi.width*roi.height-2], roi.width, { 2,1 });
	}
	
	ippsFree(edges);
	dstImg.release();
	ippsFree(edgesTS2);

	//set up ROI for BM detection
	int roiBMwidth = TSspec.bottomCut - TSspec.TSstart.x;
	IppiSize cropRoiBM = { roiBMwidth,TSspec.TSroi.height };

	/*------------------Detect Basement Membrane-------------------*/

	Ipp8u* edgesBM = selectBM(blurredIm, roi, TSspec.TSstart, cropRoiBM, edges2, PBSpeed, SpinRate,endPointsBM, keepNumBM);
	ippsFree(edges2);
	ippsFree(blurredIm);

	status = ippiDivC_8u_C1RSfs(edgesTS, roi.width, 255, TSsurfs, roi.width, roi, 0);
	status = ippiDivC_8u_C1RSfs(edgesBM, roi.width, 255, BMsurfs, roi.width, roi, 0);
	
	ippsFree(edgesBM);
	ippsFree(edgesTS);	
	ippsFree(ippIm32);
	dstT.release();
	dst.release();
	
}

topSurfSpec tissueMask(Ipp8u* srcImg, IppiSize roi,int topcut) {
	Mat src(Size(roi.width, roi.height), CV_8U, srcImg);
	IppiSize roi2 = { roi.width - topcut,roi.height };

	Ipp8u* threshImg = ippsMalloc_8u(roi.width*roi.height); //thresholded image will be placed here
	Ipp64f mean,stdDev;
	ippiMean_8u_C1R(&srcImg[topcut], roi.width, roi2, &mean); //get mean pixel intensity value 
	ippiMean_StdDev_8u_C1R(&srcImg[topcut], roi.width, roi2, &mean, &stdDev);
	Ipp32f threshold = mean+stdDev;

	//Perform thresholding using mean value as the comparison 
	ippiCompareC_8u_C1R(&srcImg[topcut], roi.width, threshold, &threshImg[topcut], roi.width, roi2, ippCmpGreater);
	//Mat thresh(Size(roi.width, roi.height), CV_8U, threshImg);
	//Morphological opening and closing
	Ipp8u* threshImg1 = morphCloseAndOpen(threshImg, roi);
	Mat thresh2(Size(roi.width, roi.height), CV_8U, threshImg1);



	//Find 8-connected regions
	int pBufSize;
	ippiLabelMarkersGetBufferSize_8u_C1R(roi, &pBufSize);
	Ipp8u* pBuffer = (Ipp8u*)ippsMalloc_8u(pBufSize);
	int pNumber = 0;
	int counts = 0;
	IppStatus status = ippiLabelMarkers_8u_C1IR(&threshImg1[topcut], roi.width, roi2, 1, 100, ippiNormInf, &pNumber, pBuffer);
	ippsFree(pBuffer);

	//Find largest connected region
	int maxCount = 0;
	int maxIdx = 0;
	for (int i = 1; i < 100; i++) {
		status = ippiCountInRange_8u_C1R(&threshImg1[topcut], roi.width, roi2, &counts, i, i);
		if (counts > maxCount) {
			maxCount = counts;
			maxIdx = i;
		}
	}
	Ipp8u* threshImg2 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	//find pixels labeled the same as maxIdx (label of the largest segment)
	status = ippiCompareC_8u_C1R(threshImg1, roi.width, maxIdx, threshImg2, roi.width, roi, ippCmpEq); 

	//Mat test(Size(roi.width, roi.height), CV_8U, threshImg1);
	//Mat test2(Size(roi.width, roi.height), CV_8U, threshImg2);
	ippsFree(threshImg);
	ippsFree(threshImg1);

	int value = 0;
	int idx = 0;
	int maxWidth = 0;
	int minWidth = roi.width;
	//Find the heighest and lowest pixel beloging to the top surface
	//To be used to crop the top surface out
	for (int row = 0; row < roi.height; row++) {
		for (int col = 0; col < roi.width; col++) {
			idx = row*roi.width + col; //offset from pointer to first element
			value = threshImg2[idx];
			if (value != 0) {
				if (col > maxWidth) {
					maxWidth = col;
				}
				if (col < minWidth) {
					minWidth = col;
				}
				break;
			}

		}
	}

	int bottomCutMin = roi.width;
	int bottomCutMax = 0;

	//Find lowest point in tissue mask region
	for (int row = 0; row < roi.height; row++) {
		for (int col = maxWidth; col < roi.width; col++) {
			idx = row*roi.width + col;
			value = threshImg2[idx];
			if (value == 0) {
				if (col > bottomCutMax) {
					bottomCutMax = col;
				}
				if (col < bottomCutMin) {
					bottomCutMin = col;
				}
				break;
			}
		}
	}
	ippsFree(threshImg2);
	thresh2.release();
	src.release();
	//Set values of topSurfSpec struct

	IppiSize topSurfROI;
	if (maxWidth - minWidth < roi.width - 10) {
		topSurfROI = { maxWidth - minWidth + 10, roi.height };
	}
	else {
		topSurfROI = { roi.width,roi.height };
	}

	IppiPoint topSurfStart;
	if (minWidth >= 5) {
		topSurfStart = { minWidth - 5,0 };
	}
	else {
		topSurfStart = { minWidth,0 };
	}

	topSurfSpecs.TSroi = topSurfROI;
	topSurfSpecs.TSstart = topSurfStart;
	topSurfSpecs.bottomCut = bottomCutMax;

	return topSurfSpecs;
}

Ipp8u* morphCloseAndOpen(Ipp8u* srcImg, IppiSize roi) {
	Ipp8u* threshImg2 = ippsMalloc_8u(roi.width*roi.height);
	Ipp8u* threshImg3 = ippsMalloc_8u(roi.width*roi.height);
	int morphBufSize, morphSpecSize;
	IppiSize maskSize = { 5,5 };
	Ipp8u pMask[5 * 5] = { 1,1,1,1,1,
		1,1,1,1,1,
		1,1,0,1,1,
		1,1,1,1,1,
		1,1,1,1,1 };


	IppStatus status = ippStsNoErr;
	IppiMorphAdvState* pMorphSpec = NULL;
	Ipp8u* pBuffer = NULL;
	Ipp8u borderValue = 0;

	status = ippiMorphAdvGetSize_8u_C1R(roi, maskSize, &morphSpecSize, &morphBufSize);

	pMorphSpec = (IppiMorphAdvState*)ippsMalloc_8u(morphSpecSize);
	pBuffer = (Ipp8u*)ippsMalloc_8u(morphBufSize);

	ippiMorphAdvInit_8u_C1R(roi, pMask, maskSize, pMorphSpec, pBuffer);

	ippiMorphCloseBorder_8u_C1R(srcImg, roi.width, threshImg2, roi.width, roi, ippBorderRepl, 0, pMorphSpec, pBuffer);


	status = ippiMorphOpenBorder_8u_C1R(threshImg2, roi.width, threshImg3, roi.width, roi, ippBorderRepl, 0, pMorphSpec, pBuffer);
	ippsFree(pBuffer);
	ippsFree(pMorphSpec);
	ippsFree(threshImg2);

	return threshImg3;

}

Ipp32f* GaussSmooth2(Ipp32f* srcImg, IppiSize roi) {
	double sigmaX = 8.2, sigmaY = 5.5; //parameters for each 1-D Gaussian kernel
	int padX = ceil(sigmaX*2.6); //determine size of padding needed for convolution
	int padY = ceil(sigmaY*2.6);
	IppiSize roiPad = { roi.width + padX * 2,roi.height + padY * 2 }; //check this
																	
	Mat image32(Size(roi.width, roi.height), CV_32F, srcImg);
	
	Mat imPad;
	//Add border to image with values reflected 
	copyMakeBorder(image32, imPad, padY, padY, padX, padX, BORDER_REFLECT);
	Ipp32f* imgPad = (Ipp32f*)&imPad.data[0];
	//x and y Gaussian kernels. Same as those used in Matlab
	Ipp32f xKernel[] = { 1.33869156620899, 1.84308876808996, 2.50007539971547, 3.34118936245775,
		4.39936709725460,5.70716519977750,7.29443761400130,9.18553198702964,11.3961443723036,
		13.9300507978285,16.7760065441748,19.9051567834374,	23.2693229707834,26.8005077120580,
		30.4118909411001,34.0004728157593,
		37.4513621005727,40.6435287976487,43.4566582741941,45.7785861134532,
		47.5126826959614,48.5845135765184,48.9471370193424,48.5845135765184,
		47.5126826959614,45.7785861134532,43.4566582741941,40.6435287976487,
		37.4513621005727,34.0004728157593,30.4118909411001,26.8005077120580,
		23.2693229707834,19.9051567834374,16.7760065441748,13.9300507978285,
		11.3961443723036,9.18553198702964,7.29443761400130,5.70716519977750,
		4.39936709725460,3.34118936245775,2.50007539971547,1.84308876808996,
		1.33869156620899, };
	Ipp32f yKernel[] = { 1.76799005691315, 2.85531503850568, 4.46140248442913,
		6.74422319494888,	9.86360385869178,	13.9566911103680,	19.1061186044955,
		25.3049585101858,	32.4251465344227,	40.1977201942300,	48.2129889116686,
		55.9461037815425,	62.8085539859661,	68.2198721063603,	71.6879505025206,
		72.8827222495024,	71.6879505025206,	68.2198721063603,	62.8085539859661,
		55.9461037815425,	48.2129889116686,	40.1977201942300,	32.4251465344227,
		25.3049585101858,	19.1061186044955,	13.9566911103680,	9.86360385869178,
		6.74422319494888,	4.46140248442913,	2.85531503850568,	1.76799005691300 };
	int divisor = 1000;

	int xKsize = sizeof(xKernel) / sizeof(xKernel[0]);
	int yKsize = sizeof(yKernel) / sizeof(yKernel[0]);
	IppiSize roiX = { xKsize,1 };
	IppiSize roiY = { 1,yKsize };

	Ipp32f* filtX = (Ipp32f*)ippsMalloc_32f(roi.width*roiPad.height);
	IppiSize roiFiltX = { roi.width,roiPad.height };
	Ipp32f* filt = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	int pBufSize;
	IppStatus status = ippiConvGetBufferSize(roiPad, roiX, ipp32f, 1, ippiROIValid, &pBufSize);
	Ipp8u* pBuffer = (Ipp8u*)ippsMalloc_8u(pBufSize);
	//Convolution of x kernel 
	status = ippiConv_32f_C1R(imgPad, roiPad.width * 4, roiPad, &xKernel[0], roiX.width * 4, roiX, filtX, roiFiltX.width * 4, ippiROIValid, pBuffer);
	status = ippiDivC_32f_C1IR(divisor, filtX, roiFiltX.width * 4, roiFiltX);
	status = ippiConvGetBufferSize(roiFiltX, roiY, ipp32f, 1, ippiROIValid, &pBufSize);
	Ipp8u* pBuffer2 = (Ipp8u*)ippsMalloc_8u(pBufSize);
	//Convolution of y kernel
	status = ippiConv_32f_C1R(filtX, roiFiltX.width * 4, roiFiltX, &yKernel[0], roiY.width * 4, roiY, filt, roi.width * 4, ippiROIValid, pBuffer2);
	//Scale result by divisor
	status = ippiDivC_32f_C1IR(divisor, filt, roi.width * 4, roi);

	imPad.release();
	ippsFree(filtX);
	ippsFree(pBuffer); ippsFree(pBuffer2);

	return filt;

}

Ipp8u* canny3(Mat srcImg, double PBspeed, double SpinRate)
{
	IppiSize roi = { srcImg.cols,srcImg.rows };
	IppiSize roiBorderX = { srcImg.cols + 2,srcImg.rows }; //pad on left and right for x-gradient kernel convolution
	IppiSize roiBorderY = { srcImg.cols,srcImg.rows + 2 }; //pad on top and bottom for y-gradient kernel convolution

	float dkernel[] = { 1.0,0.0,-1.0 }; //Differencing mask
									
	Ipp32f* kernel = (Ipp32f*)&dkernel[0];
	IppiSize roiKernX = { 3,1 }, roiKernY = { 1,3 };
	Ipp32f* ippImg = (Ipp32f*)&srcImg.data[0];
	Ipp32f* imgBrdX = (Ipp32f*)ippsMalloc_32f(roiBorderX.width*roiBorderX.height);
	Ipp32f* imgBrdY = (Ipp32f*)ippsMalloc_32f(roiBorderY.width*roiBorderY.height);
	Ipp32f* gradx = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp32f* grady = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	//add border on left and right
	IppStatus status = ippiCopyReplicateBorder_32f_C1R(ippImg, roi.width * 4, roi, imgBrdX, roiBorderX.width * 4, roiBorderX, 0, 1);
	//add border on top and bottom
	status = ippiCopyReplicateBorder_32f_C1R(ippImg, roi.width * 4, roi, imgBrdY, roiBorderY.width * 4, roiBorderY, 1, 0);
	int pBufSize2;

	/*--------------------Gradient Image------------------*/

	status = ippiConvGetBufferSize(roiKernX, roiBorderX, ipp32f, 1, ippiROIValid, &pBufSize2);
	Ipp8u* pBuffer2 = (Ipp8u*)ippsMalloc_8u(pBufSize2);
	//convolution of differencing kernel in X
	status = ippiConv_32f_C1R(kernel, roiKernX.width * 4, roiKernX, imgBrdX, roiBorderX.width * 4, roiBorderX, gradx, roi.width * 4, ippiROIValid, pBuffer2);
	status = ippiConvGetBufferSize(roiKernY, roiBorderY, ipp32f, 1, ippiROIValid, &pBufSize2);
	Ipp8u* pBuffer3 = (Ipp8u*)ippsMalloc_8u(pBufSize2);
	//convolution of differencing kernel in Y
	status = ippiConv_32f_C1R(kernel, roiKernY.width * 4, roiKernY, imgBrdY, roiBorderY.width * 4, roiBorderY, grady, roi.width * 4, ippiROIValid, pBuffer2);

	ippsFree(imgBrdY); ippsFree(imgBrdX);
	ippsFree(pBuffer2); ippsFree(pBuffer3);

	Ipp32f* gradMaskIpp = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp8u* gradMask4 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	Ipp32f* gradX2 = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp32f* gradY2 = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);

	//identify only positive x-gradient values
	status = ippiCompareC_32f_C1R(gradx, roi.width * 4, 0, gradMask4, roi.width, roi, ippCmpGreater);
	status = ippiConvert_8u32f_C1R(gradMask4, roi.width, gradMaskIpp, roi.width * 4, roi);
	//mask out regions corresponding to negative x-gradient values in both x and y grad images
	status = ippiDivC_32f_C1IR(255, gradMaskIpp, roi.width * 4, roi);
	status = ippiMul_32f_C1R(gradMaskIpp, roi.width * 4, gradx, roi.width * 4, gradX2, roi.width * 4, roi);
	status = ippiMul_32f_C1R(gradMaskIpp, roi.width * 4, grady, roi.width * 4, gradY2, roi.width * 4, roi);

	ippsFree(gradx); ippsFree(grady);

	//Scale gradient to real dimensions
	float zscale = PBspeed / SpinRate;
	float rscale = (0.004976115 / 1.33);
	status = ippiDivC_32f_C1IR(rscale, gradX2, roi.width * 4, roi);
	status = ippiDivC_32f_C1IR(zscale, gradY2, roi.width * 4, roi);

	//Combine r and z gradients, to get gradient magnitude image

	Ipp32f* gradPowX = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp32f* gradPowY = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	status = ippiSqr_32f_C1R(gradX2, roi.width * 4, gradPowX, roi.width * 4, roi);
	status = ippiSqr_32f_C1R(gradY2, roi.width * 4, gradPowY, roi.width * 4, roi);
	Ipp32f* gMag = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	status = ippiAdd_32f_C1R(gradPowY, roi.width * 4, gradPowX, roi.width * 4, gMag, roi.width * 4, roi);
	status = ippiSqrt_32f_C1IR(gMag, roi.width * 4, roi);
	Ipp32f* gcompsX = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp32f* gcompsY = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	status = ippiAddC_32f_C1IR(0.00001, gMag, roi.width * 4, roi);
	status = ippiDiv_32f_C1R(gMag, roi.width * 4, gradX2, roi.width * 4, gcompsX, roi.width * 4, roi);
	status = ippiDiv_32f_C1R(gMag, roi.width * 4, gradY2, roi.width * 4, gcompsY, roi.width * 4, roi);

	//create mesh of x,y coordinates
	Ipp32f* Xi = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp32f* Yi = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	for (int j = 0; j <= roi.height - 1; j++) {
		for (int i = 0; i <= roi.width - 1; i++) {
			status = ippiSet_32f_C1R((float)i, &Xi[j*roi.width + i], 4, { 1,1 });
			status = ippiSet_32f_C1R((float)j, &Yi[j*roi.width + i], 4, { 1,1 });
		}
	}

	/*--------------------Non-maximum Suppression------------------*/

	Ipp32f* X2i = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp32f* X3i = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp32f* Y2i = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp32f* Y3i = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
		
	//get interpolant in the gradient direction 
	status = ippiAdd_32f_C1R(Xi, roi.width * 4, gcompsX, roi.width * 4, X2i, roi.width * 4, roi);
	status = ippiAdd_32f_C1R(Yi, roi.width * 4, gcompsY, roi.width * 4, Y2i, roi.width * 4, roi);
	//get interpolant opposite to the gradient direction
	status = ippiSub_32f_C1R(gcompsX, roi.width * 4, Xi, roi.width * 4, X3i, roi.width * 4, roi);
	status = ippiSub_32f_C1R(gcompsY, roi.width * 4, Yi, roi.width * 4, Y3i, roi.width * 4, roi);
	status = ippiMul_32f_C1IR(gradMaskIpp, roi.width * 4, X2i, roi.width * 4, roi);
	status = ippiMul_32f_C1IR(gradMaskIpp, roi.width * 4, Y2i, roi.width * 4, roi);
	status = ippiMul_32f_C1IR(gradMaskIpp, roi.width * 4, X3i, roi.width * 4, roi);
	status = ippiMul_32f_C1IR(gradMaskIpp, roi.width * 4, Y3i, roi.width * 4, roi);

	IppiRect roi2 = { 0,0,roi.width,roi.height };
	Ipp32f* gintA = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp32f* gintB = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	int interpFlag = IPPI_INTER_LINEAR | IPPI_SMOOTH_EDGE;
	//interpolate in the gradient direction
	status = ippiRemap_32f_C1R(gMag, roi, roi.width * 4, roi2, X2i, roi.width * 4, Y2i, roi.width * 4, gintA, roi.width * 4, roi, interpFlag);
	//interpolate opposite to the gradient direction
	status = ippiRemap_32f_C1R(gMag, roi, roi.width * 4, roi2, X3i, roi.width * 4, Y3i, roi.width * 4, gintB, roi.width * 4, roi, interpFlag);

	Ipp8u* edges1 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	Ipp8u* edges2 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	//suppress nonmax pixels
	status = ippiCompare_32f_C1R(gintA, roi.width * 4, gMag, roi.width * 4, edges1, roi.width, roi, ippCmpGreaterEq);
	status = ippiNot_8u_C1IR(edges1, roi.width, roi);
	status = ippiCompare_32f_C1R(gintB, roi.width * 4, gMag, roi.width * 4, edges2, roi.width, roi, ippCmpGreaterEq);
	status = ippiNot_8u_C1IR(edges2, roi.width, roi);
	Ipp8u* edges = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	status = ippiAnd_8u_C1R(edges1, roi.width, edges2, roi.width, edges, roi.width, roi);
	//Mat ed1(Size(roi.width, roi.height), CV_8U, edges);

	ippsFree(gradX2);
	ippsFree(gradY2);

	ippsFree(gradMaskIpp);
	ippsFree(gradMask4);
	ippsFree(gradPowX);
	ippsFree(gradPowY);
	ippsFree(gcompsX);
	ippsFree(gcompsY);
	ippsFree(Xi);
	ippsFree(Yi);
	ippsFree(X2i); ippsFree(Y2i); ippsFree(X3i); ippsFree(Y3i);
	ippsFree(gintA); ippsFree(gintB);
	ippsFree(edges1); ippsFree(edges2);

	/*--------------------Hysteresis Thresholding------------------*/

	//Find high and low thresholds 
	//Determined using percentiles of pixel value distribution
	Ipp8u* gradMask2 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	status = ippiCompareC_32f_C1R(gMag, roi.width * 4, 0.001, gradMask2, roi.width, roi, ippCmpGreater);
	Mat gradMaskCV(Size(roi.width, roi.height), CV_8U, gradMask2);

	Mat locations;
	findNonZero(gradMaskCV, locations);
	gradMaskCV.release();
	Mat pixVal(1, locations.rows, CV_32F);
	Ipp32u* locationsIpp;
	locationsIpp = (Ipp32u*)&locations.data[0];
	float* p = (float*)pixVal.ptr();
	//put all non-zero pixels into an array
	for (int i = 0; i < locations.rows; i++) {
		int indx = locationsIpp[i * 2 + 1] * roi.width + locationsIpp[i * 2];
		p[i] = gMag[indx];
	}

	Ipp32f* pixIpp = (Ipp32f*)&pixVal.data[0];
	status = ippsSortAscend_32f_I(pixIpp, locations.rows); //sort values in ascending order
	locations.release();

	float loPct = 40, hiPct = 55; 
	int ptIndLo = floor(((loPct / 100)*pixVal.cols) + 0.5);
	int ptIndHi = floor(((hiPct / 100)*pixVal.cols) + 0.5);

	float loThresh = pixVal.at<float>(0, ptIndLo);
	float hiThresh = pixVal.at<float>(0, ptIndHi);
	pixVal.release();

	Ipp32f* edges32 = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	status = ippiConvert_8u32f_C1R(edges, roi.width, edges32, roi.width * 4, roi);
	status = ippiDivC_32f_C1IR(255, edges32, roi.width * 4, roi);
	//mask gMag with the edge image
	Ipp32f* ge = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	status = ippiMul_32f_C1R(gMag, roi.width * 4, edges32, roi.width * 4, ge, roi.width * 4, roi);

	ippsFree(edges32);
	ippsFree(edges);
	ippsFree(gMag);

	Ipp8u* e1 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	Ipp8u* e2 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);

	status = ippiCompareC_32f_C1R(ge, roi.width * 4, loThresh, e1, roi.width, roi, ippCmpGreaterEq); //apply low threshold
	status = ippiCompareC_32f_C1R(ge, roi.width * 4, hiThresh, e2, roi.width, roi, ippCmpGreaterEq); //apply high threshold

	Ipp8u* e3 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	
	int pBufSize;
	status = ippiLabelMarkersGetBufferSize_8u_C1R(roi, &pBufSize);
	Ipp8u* pBuffer = (Ipp8u*)ippsMalloc_8u(pBufSize);
	int pNumber;
	//find connected components of edges passing the thresholds
	status = ippiLabelMarkers_8u_C1IR(e1, roi.width, roi, 1, 200, ippiNormInf, &pNumber, pBuffer); 
	status = ippiLabelMarkers_8u_C1IR(e2, roi.width, roi, 1, 200, ippiNormInf, &pNumber, pBuffer);
	status = ippiAdd_8u_C1RSfs(e1, roi.width, e2, roi.width, e3, roi.width, roi, 0);
	ippsFree(pBuffer);
	ippsFree(e2);
	Mat conn1(Size(roi.width, roi.height), CV_8U, e1);

	//does what imreconstruct does in matlab
	//part of hysteresis thresholding
	double min, max;
	Ipp8u min2, max2, min3, max3;
	minMaxLoc(conn1, &min, &max);
	conn1.release();

	Ipp8u* e4 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	Ipp8u* e5 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	Ipp8u* e1masked = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	Ipp8u* e3masked = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);

	bool keepLine = true;
	
	Ipp8u* dstImg = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	status = ippsZero_8u(dstImg, roi.width*roi.height);

	Mat dstIm(Size(roi.width, roi.height), CV_8U, dstImg);
	float *p1, *p2;

	int idx;
	for (idx = 1; idx <= max; idx++) {
		//isolate the connected component with marker equal to idx
		status = ippiCompareC_8u_C1R(e1, roi.width, idx + 1, e4, roi.width, roi, ippCmpLess);
		status = ippiCompareC_8u_C1R(e1, roi.width, idx - 1, e5, roi.width, roi, ippCmpGreater);
		status = ippiMul_8u_C1IRSfs(e4, roi.width, e5, roi.width, roi, 0); //only the marker = idx in e1 will pass through this multiplication
		status = ippiDivC_8u_C1IRSfs(255, e5, roi.width, roi, 0);
		status = ippiMul_8u_C1RSfs(e5, roi.width, e1, roi.width, e1masked, roi.width, roi, 0); //restore marker number in isolated image
		status = ippiMul_8u_C1RSfs(e5, roi.width, e3, roi.width, e3masked, roi.width, roi, 0); //restore marker number(s) in isolated image. May be multiple multiple numbers
		status = ippiMax_8u_C1R(e1masked, roi.width, roi, &max2); //max is just the marker number of the line in the image
		status = ippiMax_8u_C1R(e3masked, roi.width, roi, &max3); 

		//
		if (max2 != max3) {
			status = ippiAdd_8u_C1IRSfs(e5, roi.width, dstImg, roi.width, roi, 0);
		}
	}

	//Mat dst1(Size(roi.width, roi.height), CV_8U, dstImg);
	status = ippiMulC_8u_C1IRSfs(255, dstImg, roi.width, roi, 0);

	ippsFree(gradMask2);
	ippsFree(ge);
	ippsFree(e1);
	ippsFree(e3);
	ippsFree(e4);
	ippsFree(e5);
	ippsFree(e1masked);
	ippsFree(e3masked);

	return dstImg;

}

Ipp8u* selectEpith(Ipp32f* srcImg, IppiSize srcRoi, IppiPoint startPoint, IppiSize TScropROI, Ipp8u* edges, double PBspeed, double SpinRate,Ipp32f* endpoints,int* keepNumTS) {
	float rpref = 10;
	float zscale = PBspeed / SpinRate;
	float rscale = (0.004976115 / 1.33);
	
	IppiSize roi = TScropROI;
	float data[] = { 0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		1,
		0,
		1,
		1,
		1,
		1,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		1,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		1,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		1,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		1,
		1,
		1,
		1,
		0,
		1,
		1,
		1,
		1,
		0 };
	Mat LUT(512, 1, CV_32F, &data); //create look-up table for finding endpoints
	LUT.convertTo(LUT, CV_8UC1);
	
	int pBufSize;
	int a = startPoint.y*srcRoi.width + startPoint.x;
	Ipp8u* startAddr = &edges[startPoint.y*srcRoi.width + startPoint.x];
	IppStatus status = ippiLabelMarkersGetBufferSize_8u_C1R(roi, &pBufSize);
	int  pNumber;
	Ipp8u* pBuffer = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);

	//find connected components
	//labels each connected region with a different number identifier
	Ipp8u* edgesCopy = (Ipp8u*)ippsMalloc_8u(srcRoi.width*srcRoi.height);
	status = ippiCopy_8u_C1R(edges, srcRoi.width, edgesCopy, srcRoi.width, srcRoi);
	status = ippiLabelMarkers_8u_C1IR(startAddr, srcRoi.width, roi, 2, 200, ippiNormInf, &pNumber, pBuffer);
	status = ippiSubC_8u_C1IRSfs(1, edges, srcRoi.width, srcRoi, 0);
	ippsFree(pBuffer);
	Ipp8u* nonLabeled = (Ipp8u*)ippsMalloc_8u(srcRoi.width*srcRoi.height);
	status = ippiCompareC_8u_C1R(edges, srcRoi.width, 254, nonLabeled, srcRoi.width, srcRoi, ippCmpEq);
	status = ippiNot_8u_C1IR(nonLabeled, srcRoi.width, srcRoi);
	status = ippiDivC_8u_C1IRSfs(255, nonLabeled, srcRoi.width, srcRoi, 0);
	status = ippiMul_8u_C1IRSfs(nonLabeled, srcRoi.width, edges, srcRoi.width, srcRoi, 0);
	ippsFree(nonLabeled);
	

	//Find largest line segment
	int counts;
	int indx = 0, max = 0;
	for (int i = 1; i <= pNumber; i++) {
		status = ippiCountInRange_8u_C1R(edges, srcRoi.width, srcRoi, &counts, i, i);
		if (counts > max) {
			max = counts;
			indx = i;
		}
	}

	Ipp32f* x1 = (Ipp32f*)ippsMalloc_32f(pNumber);
	Ipp32f* x2 = (Ipp32f*)ippsMalloc_32f(pNumber);
	Ipp32f* y1 = (Ipp32f*)ippsMalloc_32f(pNumber);
	Ipp32f* y2 = (Ipp32f*)ippsMalloc_32f(pNumber);

	Ipp8u val = 0;
	
	IppiSize roiOne = { 1,1 };
	int32_t x1val, x2val, y1val, y2val;
	int maxSize = 0;
	Ipp8u* endPoints = (Ipp8u*)ippsMalloc_8u(srcRoi.width*srcRoi.height);
	Mat nonZeroCoord;
	Ipp32u* nonZeroIpp;
	Ipp8u* endPtemp = (Ipp8u*)ippsMalloc_8u(srcRoi.width*srcRoi.height);
	Mat endPts(Size(srcRoi.width, srcRoi.height), CV_8U, endPtemp);
	Ipp8u* maskLab = (Ipp8u*)ippsMalloc_8u(srcRoi.width*srcRoi.height);
	
	bwlookup2(edges, endPoints, LUT, BORDER_CONSTANT, 0, srcRoi);
	LUT.release();
	Ipp32f* endP1 = (Ipp32f*)ippsMalloc_32f(srcRoi.width*srcRoi.height);

	//Find the endpoints of all clumps of pixels found previously
	int lastRow;
	int labelNum;
	for (labelNum = 1; labelNum <= pNumber; labelNum++)
	{
		
		status = ippsZero_8u(endPtemp, srcRoi.width*srcRoi.height); //re-initialize the temporary buffer with zeros
		status = ippiCompareC_8u_C1R(edges, srcRoi.width, labelNum, maskLab, srcRoi.width, srcRoi, ippCmpEq);
		status = ippiMul_8u_C1RSfs(maskLab, srcRoi.width, endPoints, srcRoi.width, endPtemp, srcRoi.width, srcRoi, 0);
		findNonZero(endPts, nonZeroCoord);
		nonZeroIpp = (Ipp32u*)&nonZeroCoord.data[0];
		lastRow = nonZeroCoord.rows;
		if (nonZeroCoord.rows > 1)
		{
			x1val = nonZeroIpp[0];
			y1val = nonZeroIpp[1];
			x2val = nonZeroIpp[(lastRow - 1) * 2];
			y2val = nonZeroIpp[(lastRow - 1) * 2 + 1];
			status = ippiSet_32f_C1R(x1val, &x1[labelNum - 1], 1, roiOne);
			status = ippiSet_32f_C1R(y1val, &y1[labelNum - 1], 1, roiOne);
			status = ippiSet_32f_C1R(x2val, &x2[labelNum - 1], 1, roiOne);
			status = ippiSet_32f_C1R(y2val, &y2[labelNum - 1], 1, roiOne);
		}
		else if (nonZeroCoord.rows==1){
			x1val = nonZeroIpp[0];
			y1val = nonZeroIpp[1];
			x2val = nonZeroIpp[0];
			y2val = nonZeroIpp[1];
			status = ippiSet_32f_C1R(x1val, &x1[labelNum - 1], 1, roiOne);
			status = ippiSet_32f_C1R(y1val, &y1[labelNum - 1], 1, roiOne);
			status = ippiSet_32f_C1R(x2val, &x2[labelNum - 1], 1, roiOne);
			status = ippiSet_32f_C1R(y2val, &y2[labelNum - 1], 1, roiOne);
		}

		status = ippiConvert_32u32f_C1R(nonZeroIpp, 2 * 4, &endP1[(labelNum - 1) * 4], 2 * 4, { 2,nonZeroCoord.rows });
		
	}
	
	ippsFree(maskLab);
	ippsFree(endPtemp);
	nonZeroCoord.release();
	ippsFree(endPoints);
	ippsFree(edgesCopy);

	//look to the right of the largest line
	int conIndLine = indx, less = 1, k = 0, countMinI;
	float length, minLength, r1, z1, r2, z2;
	int minI = conIndLine;
	vector<int> keepIndL;
	vector<int> keepIndR;
	int keepNumL = 0, keepNumR = 0; //count how many numbers are kept to the left and right
	
	while (less == 1)
	{
		r1 = x2[conIndLine - 1];
		z1 = y2[conIndLine - 1];
		minLength = (srcRoi.height - z1 - 1)*zscale;
		less = 0;
		for (int j = 1; j <= pNumber; j++)
		{
			if (j != conIndLine)
			{
				if ((y2[j - 1] * zscale > z1*zscale) && (y1[j - 1] * zscale != y2[j - 1] * zscale))
				{
					z2 = pow((z1 - y1[j - 1])*zscale, 2);
					r2 = pow((r1 - x1[j - 1])*rscale*rpref, 2);

					length = sqrt(r2 + z2);

					if (length < minLength)
					{
						minI = j;
						minLength = length;
						less = 1;
					}
					//if length is equal, take the longer line
					else if (length == minLength)
					{
						status = ippiCountInRange_8u_C1R(edges, srcRoi.width, srcRoi, &counts, j, j);
						status = ippiCountInRange_8u_C1R(edges, srcRoi.width, srcRoi, &countMinI, minI, minI);
						if (counts > countMinI)
						{
							minI = j;
							minLength = length;
							less = 1;
						}
					}
				}
			}
		}

		if (conIndLine != minI && k<=pNumber)
		{
			//keepIndR[k] = minI;
			keepIndR.push_back(minI);
			status = ippiCopy_32f_C1R(&endP1[(minI - 1) * 4], 4, &endpoints[keepNumR * 4], 4, { 1,4 });
			keepNumR++;
		}
		conIndLine = minI;
		k++;
	}
	
	status = ippiCopy_32f_C1R(&endP1[(indx - 1) * 4], 4, &endpoints[keepNumR * 4], 4, { 1,4 });

	//Look to the left of the largest line
	conIndLine = indx; less = 1; k = 0; minI = conIndLine;
	while (less == 1)
	{
		r1 = x1[conIndLine - 1];
		z1 = y1[conIndLine - 1];
		minLength = z1*zscale;
		less = 0;
		for (int j = 1; j <= pNumber; j++)
		{
			if (j != conIndLine)
			{
				if ((y1[j - 1] * zscale < z1*zscale) && (y1[j - 1] * zscale != y2[j - 1] * zscale))
				{
					z2 = pow((z1 - y2[j - 1])*zscale, 2);
					r2 = pow((r1 - x2[j - 1])*rscale*rpref, 2);

					length = sqrt(r2 + z2);

					if (length < minLength)
					{
						minI = j;
						minLength = length;
						less = 1;
					}
					//if length is equal, take the longer line
					else if (length == minLength)
					{
						status = ippiCountInRange_8u_C1R(edges, srcRoi.width, srcRoi, &counts, j, j);
						status = ippiCountInRange_8u_C1R(edges, srcRoi.width, srcRoi, &countMinI, minI, minI);
						if (counts > countMinI)
						{
							minI = j;
							minLength = length;
							less = 1;
						}
					}
				}
			}
		}

		if (conIndLine != minI && k<=pNumber)
		{
			keepIndL.push_back(minI);
			status = ippiCopy_32f_C1R(&endP1[(minI - 1) * 4], 4, &endpoints[(keepNumR + keepNumL + 1) * 4], 4, { 1,4 });
			keepNumL++;
		}
		conIndLine = minI;
		k++;
	}

	//create new image with only selected lines
	Ipp8u* newmat = (Ipp8u*)ippsMalloc_8u(srcRoi.width*srcRoi.height);
	Ipp8u* tempmat = (Ipp8u*)ippsMalloc_8u(srcRoi.width*srcRoi.height);

	//start with the largest line
	status = ippiCompareC_8u_C1R(edges, srcRoi.width, indx, newmat, srcRoi.width, srcRoi, ippCmpEq);

	//include lines to left
	if (keepNumL > 0) {
		for (int i = 0; i <= keepNumL - 1; i++)
		{
			status = ippiCompareC_8u_C1R(edges, srcRoi.width, keepIndL[i], tempmat, srcRoi.width, srcRoi, ippCmpEq);
			status = ippiAdd_8u_C1IRSfs(tempmat, srcRoi.width, newmat, srcRoi.width, srcRoi, 0);
		}

	}

	//include lines to right
	if (keepNumR > 0) {
		for (int i = 0; i <= keepNumR - 1; i++)
		{
			status = ippiCompareC_8u_C1R(edges, srcRoi.width, keepIndR[i], tempmat, srcRoi.width, srcRoi, ippCmpEq);
			status = ippiAdd_8u_C1IRSfs(tempmat, srcRoi.width, newmat, srcRoi.width, srcRoi, 0);
		}

	}

	keepNumTS[0] = keepNumR + keepNumL + 1;

	ippsFree(x1); ippsFree(x2); ippsFree(y1); ippsFree(y2);
	ippsFree(tempmat);
	ippsFree(endP1);
	
	return newmat;
}

Ipp8u* selectBM(Ipp32f* srcImg, IppiSize srcRoi, IppiPoint startPoint, IppiSize BMcropROI, Ipp8u* edges, double PBspeed, double SpinRate, Ipp32f* endpoints, int* keepNumBM) {
	float rpref = 10.0;
	float zscale = PBspeed / SpinRate;
	float rscale = (0.004976115 / 1.33);
	float overlapTol = -0.9;
	IppiSize roi = srcRoi;
	IppiSize roiCr = BMcropROI;
	float data[] = { 0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		1,
		1,
		1,
		0,
		1,
		1,
		1,
		1,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		1,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		1,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		1,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		1,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		1,
		0,
		0,
		0,
		1,
		0,
		1,
		1,
		1,
		1,
		0,
		1,
		1,
		1,
		1,
		0 };
	Mat LUT(512, 1, CV_32F, &data); //create look-up table for finding endpoints
	LUT.convertTo(LUT, CV_8UC1);
	IppStatus status;
	int pBufSize;

	status= ippiLabelMarkersGetBufferSize_8u_C1R(roi, &pBufSize);
	int  pNumber;
	Ipp8u* pBuffer = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);

	//find connected components
	//labels each connected region with a different number identifier
	Ipp32s* edges32s = (Ipp32s*)ippsMalloc_32s(roi.width*roi.height);
	Ipp32f* edges32f = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp32f* edges32f2 = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	status = ippiLabelMarkers_8u32s_C1R(edges, roi.width, edges32s,roi.width*4, roi, 2, 5000, ippiNormInf, &pNumber, pBuffer);
	status = ippiConvert_32s32f_C1R(edges32s, roi.width * 4, edges32f, roi.width * 4, roi);
	status = ippiSubC_32f_C1R(edges32f, roi.width * 4, 1, edges32f2, roi.width * 4, roi);
	Ipp8u* posMask = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	status = ippiCompareC_32f_C1R(edges32f2, roi.width * 4, 0, posMask, roi.width, roi, ippCmpGreaterEq);
	status = ippiDivC_8u_C1IRSfs(255, posMask, roi.width, roi, 0);
	status = ippiConvert_8u32f_C1R(posMask, roi.width, edges32f, roi.width * 4, roi);
	status = ippiMul_32f_C1IR(edges32f, roi.width * 4, edges32f2, roi.width * 4, roi);
	Mat test4(Size(roi.width, roi.height), CV_32F, srcImg);
	
	ippsFree(edges32s);
	ippsFree(edges32f);
	
	ippsFree(pBuffer);
	ippsFree(posMask);

	//Find largest line segment
	int counts;
	int indx = 0, max = 0;
	for (int i = 1; i <= pNumber; i++) {
		status = ippiCountInRange_32f_C1R(edges32f2, roi.width*4, roi, &counts, i, i);
		if (counts > max) {
			max = counts;
			indx = i;
		}
	}

	Ipp32f* x1 = (Ipp32f*)ippsMalloc_32f(pNumber);
	Ipp32f* x2 = (Ipp32f*)ippsMalloc_32f(pNumber);
	Ipp32f* y1 = (Ipp32f*)ippsMalloc_32f(pNumber);
	Ipp32f* y2 = (Ipp32f*)ippsMalloc_32f(pNumber);

	Ipp8u val = 0;
	
	//Initialize buffers and variables
	Mat nonZeroCoord;
	Ipp32u* nonZeroIpp;
	IppiSize roiOne = { 1,1 };
	int32_t x1val, x2val, y1val, y2val;
	int maxSize = 0;
	Ipp8u* endPoints = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	Ipp8u* endPtemp = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	Ipp32f* nonZero2 = (Ipp32f*)ippsMalloc_32f(10);
	Ipp8u* maskLab = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	Ipp32f* endP1 = (Ipp32f*)ippsMalloc_32f(srcRoi.width*srcRoi.height);
	
	
	bwlookup2(edges, endPoints, LUT, BORDER_CONSTANT, 0, srcRoi);
	LUT.release();
	int numRows;
	int lastRow;
	int labelNum;

	for ( labelNum = 1; labelNum <= pNumber; labelNum++)
	{
		status = ippsZero_8u(endPtemp, srcRoi.width*srcRoi.height);
		status = ippsZero_8u(endPtemp, srcRoi.width*srcRoi.height); //re-initialize the temporary buffer with zeros
		status = ippiCompareC_32f_C1R(edges32f2, srcRoi.width*4, labelNum, maskLab, srcRoi.width, srcRoi, ippCmpEq);
		status = ippiMul_8u_C1RSfs(maskLab, srcRoi.width, endPoints, srcRoi.width, endPtemp, srcRoi.width, srcRoi, 0);
		findNonZero2(endPtemp, nonZero2, roi, &numRows);
		x1val = nonZero2[0];
		y1val = nonZero2[1];
		x2val = nonZero2[2];
		y2val = nonZero2[3];
		status = ippiSet_32f_C1R(x1val, &x1[labelNum - 1], 1, roiOne);
		status = ippiSet_32f_C1R(y1val, &y1[labelNum - 1], 1, roiOne);
		status = ippiSet_32f_C1R(x2val, &x2[labelNum - 1], 1, roiOne);
		status = ippiSet_32f_C1R(y2val, &y2[labelNum - 1], 1, roiOne);

		status = ippiCopy_32f_C1R(nonZero2, 2 * 4, &endP1[(labelNum - 1) * 4], 2 * 4, { 2,2 });
		
	}
	ippsFree(maskLab);
	ippsFree(nonZero2);
	ippsFree(endPoints); 
	ippsFree(endPtemp);
	nonZeroCoord.release();
	
	//look to the right of the largest line
	int conIndLine = indx, less = 1, k = 0, countMinI;
	float length, minLength, r1, z1, r2, z2;
	int minI = conIndLine;
	int lastMinI;
	vector<int> keepIndL;
	vector<int> keepIndR;

	int keepNumL = 0, keepNumR = 0; //count how many numbers are kept to the left and right
	while (less == 1)
	{
		r1 = x1[conIndLine - 1];
		z1 = y1[conIndLine - 1];
		minLength = z1*zscale;
		less = 0;
		for (int j = 1; j <= pNumber; j++)
		{
			if (j != conIndLine)
			{
				if ((y1[j - 1] * zscale < z1*zscale) && (y1[j - 1] * zscale != y2[j - 1] * zscale) && z1*zscale - y2[j - 1] * zscale>overlapTol)
				{
					z2 = pow((z1 - y2[j - 1])*zscale, 2);
					r2 = pow((r1 - x2[j - 1])*rscale*rpref, 2);

					length = sqrt(r2 + z2);

					if (length < minLength)
					{
						minI = j;
						minLength = length;
						less = 1;
					}
					//if length is equal, take the longer line
					else if (length == minLength)
					{
						status = ippiCountInRange_8u_C1R(edges, srcRoi.width, srcRoi, &counts, j, j);
						status = ippiCountInRange_8u_C1R(edges, srcRoi.width, srcRoi, &countMinI, minI, minI);
						if (counts > countMinI)
						{
							minI = j;
							minLength = length;
							less = 1;
						}
					}
				}
			}
		}

		if (conIndLine != minI && k<=pNumber)
		{
			keepIndR.push_back(minI);
			status = ippiCopy_32f_C1R(&endP1[(minI-1)*4], 4, &endpoints[keepNumR*4], 4, { 1,4 });
			keepNumR++;
		}
		conIndLine = minI;
		k++;
	}
	status = ippiCopy_32f_C1R(&endP1[(indx - 1) * 4], 4, &endpoints[keepNumR * 4], 4, { 1,4 });

	//Look to the left of the largest line
	conIndLine = indx; less = 1; k = 0; minI = conIndLine;

	while (less == 1)
	{
		r1 = x2[conIndLine - 1];
		z1 = y2[conIndLine - 1];
		minLength = (srcRoi.height - z1 - 1)*zscale;
		less = 0;
		for (int j = 1; j <= pNumber; j++)
		{
			if (j != conIndLine)
			{
				if ((y2[j - 1] * zscale > z1*zscale) && (y1[j - 1] * zscale != y2[j - 1] * zscale) && y1[j - 1] * zscale - z1*zscale > overlapTol)
				{
					z2 = pow((z1 - y1[j - 1])*zscale, 2);
					r2 = pow((r1 - x1[j - 1])*rscale*rpref, 2);

					length = sqrt(r2 + z2);

					if (length < minLength)
					{
						minI = j;
						minLength = length;
						less = 1;
					}
					//if length is equal, take the longer line
					else if (length == minLength)
					{
						status = ippiCountInRange_8u_C1R(edges, srcRoi.width, srcRoi, &counts, j, j);
						status = ippiCountInRange_8u_C1R(edges, srcRoi.width, srcRoi, &countMinI, minI, minI);
						if (counts > countMinI)
						{
							minI = j;
							minLength = length;
							less = 1;
						}
					}
				}
			}
		}

		if (conIndLine != minI && k<=pNumber)
		{
			keepIndL.push_back(minI);
			status = ippiCopy_32f_C1R(&endP1[(minI - 1) * 4], 4, &endpoints[(keepNumR+keepNumL+1) * 4], 4, { 1,4 });
			keepNumL++;
		}
		conIndLine = minI;
		k++;
	}
	ippsFree(endP1);

	//create new image with only selected lines
	Ipp8u* newmat = (Ipp8u*)ippsMalloc_8u(srcRoi.width*srcRoi.height);
	Ipp8u* tempmat = (Ipp8u*)ippsMalloc_8u(srcRoi.width*srcRoi.height);
	
	//start with the largest line
	
	status = ippiCompareC_32f_C1R(edges32f2, srcRoi.width*4, indx, newmat, srcRoi.width, srcRoi, ippCmpEq);

	//include lines to left
	if (keepNumL > 0) {
		for (int i = 0; i <= keepNumL - 1; i++)
		{
			status = ippiCompareC_32f_C1R(edges32f2, srcRoi.width * 4, keepIndL[i], tempmat, srcRoi.width, srcRoi, ippCmpEq);
			status = ippiAdd_8u_C1IRSfs(tempmat, srcRoi.width, newmat, srcRoi.width, srcRoi, 0);
		}

	}

	//include lines to right
	if (keepNumR > 0) {
		for (int i = 0; i <= keepNumR - 1; i++)
		{
			status = ippiCompareC_32f_C1R(edges32f2, srcRoi.width * 4, keepIndR[i], tempmat, srcRoi.width, srcRoi, ippCmpEq);
			status = ippiAdd_8u_C1IRSfs(tempmat, srcRoi.width, newmat, srcRoi.width, srcRoi, 0);
		}

	}

	keepNumBM[0] = keepNumR + keepNumL + 1;
	ippsFree(x1);
	ippsFree(x2);
	ippsFree(y1);
	ippsFree(y2);
	ippsFree(edges32f2);
	ippsFree(tempmat);
	test4.release();

	return newmat;
}

void bwlookup2(Ipp8u* inIpp, Ipp8u* outIpp, const cv::Mat & lut, int bordertype, cv::Scalar px, IppiSize roi)
{

	const unsigned char * _lut = lut.data;

	Ipp8u* inIpp2 = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	IppiSize roiCopy = { roi.width + 2,roi.height + 2 };
	Ipp8u* inBord = (Ipp8u*)ippsMalloc_8u(roiCopy.width*roiCopy.height);

	IppStatus status = ippiCopy_8u_C1R(inIpp, roi.width, inIpp2, roi.width, roi);

	status = ippiCopyConstBorder_8u_C1R(inIpp, roi.width, roi, inBord, roiCopy.width, roiCopy, 1, 1, 0);


	const int rows = roi.height + 1;
	const int cols = roi.width + 1;
	for (int y = 1; y<rows; ++y)
	{
		for (int x = 1; x<cols; ++x)
		{
			int L = 0;
			const int jmax = y + 1;

			for (int j = y - 1, k = 1; j <= jmax; ++j, k <<= 1)
			{

				Ipp8u* p = &inBord[j*roiCopy.width + x - 1];

				for (unsigned int u = 0; u<3; ++u)
				{
					if (p[u])
						L += (k << 3 * u);
				}
			}

			outIpp[(y - 1)*roi.width + x - 1] = _lut[L];
		}
	}
	ippsFree(inIpp2); ippsFree(inBord);

}

void avgLocs(int *locs, int indx, int dep) {

	int diff;
	if (indx < 5) {
		diff = 5 - indx;
		int j;
		for (j = 0; j < diff; j++) {
			locs[j] = 0;
		}
		int k = 0;
		for (int i = j; i <= 11; i++) {
			locs[i] = k;
			k++;
		}
	}
	else if (dep - indx - 1 < 5) {
		diff = 5 - dep + indx + 1;
		int j;
		for (j = 0; j < diff; j++) {
			locs[10 - j] = dep - 1;
		}
		int k = dep - 1;
		for (int i = j; i <= 11; i++) {
			locs[10 - i] = k;
			k--;
		}
	}
	else {
		int k = indx - 5;
		for (int i = 0; i <= 11; i++) {
			locs[i] = k;
			k++;
		}
	}
}

void findNonZero2(Ipp8u* endPoints, Ipp32f* nonZeroCoords, IppiSize roi, int* numPoints) {

	Mat labs(Size(roi.width, roi.height), CV_8U, endPoints);
	IppStatus status;
	Ipp32s* endLabs = (Ipp32s*)ippsMalloc_32s(roi.width*roi.height);
	Ipp32f* endLabs32f = (Ipp32f*)ippsMalloc_32f(roi.width*roi.height);
	Ipp8u* zeroPix = (Ipp8u*)ippsMalloc_8u(roi.width*roi.height);
	int pBufSize, pNumber;
	status = ippiLabelMarkersGetBufferSize_8u_C1R(roi, &pBufSize);
	Ipp8u* pBuffer = (Ipp8u*)ippsMalloc_8u(pBufSize);
	status = ippiLabelMarkers_8u_C1IR(endPoints, roi.width, roi, 1, 100, ippiNormL1, &pNumber, pBuffer);
	status = ippiCompareC_8u_C1R(endPoints, roi.width, 0, zeroPix, roi.width, roi, ippCmpEq);
	status = ippiMulC_8u_C1IRSfs(101, zeroPix, roi.width, roi, 0);
	Ipp8u maxPix, minPix;
	int xMax, yMax, xMin, yMin;
	status = ippiMaxIndx_8u_C1R(endPoints, roi.width, roi, &maxPix, &xMax, &yMax);
	status = ippiAdd_8u_C1IRSfs(zeroPix, roi.width, endPoints, roi.width, roi, 0);
	status = ippiMinIndx_8u_C1R(endPoints, roi.width, roi, &minPix, &xMin, &yMin);
	
	nonZeroCoords[0] = xMin;
	nonZeroCoords[1] = yMin;
	nonZeroCoords[2] = xMax;
	nonZeroCoords[3] = yMax;

	numPoints[0] = 2;

	labs.release();
	ippsFree(endLabs);
	ippsFree(endLabs32f);
	ippsFree(pBuffer);
	ippsFree(zeroPix);
}

void enfaceCutoffs(Ipp32f* enface, IppiSize roiEnface, Ipp8u* enfaceMask, int* centerFrame) {
	IppStatus status;

	Mat enfcArr(Size(1, roiEnface.height*roiEnface.width), CV_32F, enface);
	Mat labels;
	Mat centers;

	kmeans(enfcArr, 4, labels, TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 10, 1.0), 1, KMEANS_PP_CENTERS, centers);
	Mat labels2(Size(roiEnface.width, roiEnface.height), CV_32S, &labels.data[0]);
	



	Ipp32s* kmLabels = (Ipp32s*)&labels2.data[0];
	Ipp32f* cntrs = (Ipp32f*)&centers.data[0];
	Ipp32f pMin1, pMin2, pMax, pMax2;
	int pIndx1, pIndx2;
	status = ippsMin_32f(cntrs, 4, &pMin1);
	status = ippsMaxIndx_32f(cntrs, 4, &pMax, &pIndx1);
	status = ippsSet_32f(pMin1, &cntrs[pIndx1], 1);
	status = ippsMaxIndx_32f(cntrs, 4, &pMax2, &pIndx2);
	Ipp8u* kmThresh1 = (Ipp8u*)ippsMalloc_8u(roiEnface.width*roiEnface.height);
	Ipp8u* kmThresh2 = (Ipp8u*)ippsMalloc_8u(roiEnface.width*roiEnface.height);
	Ipp32f* km32 = (Ipp32f*)ippsMalloc_32f(roiEnface.width*roiEnface.height);
	status = ippiConvert_32s32f_C1R(kmLabels, roiEnface.width * 4, km32, roiEnface.width * 4, roiEnface);
	status = ippiCompareC_32f_C1R(km32, roiEnface.width * 4, pIndx1, kmThresh1, roiEnface.width, roiEnface, ippCmpEq);
	status = ippiCompareC_32f_C1R(km32, roiEnface.width * 4, pIndx2, kmThresh2, roiEnface.width, roiEnface, ippCmpEq);
	status = ippiAdd_8u_C1RSfs(kmThresh2, roiEnface.width, kmThresh1, roiEnface.width, enfaceMask,roiEnface.width, roiEnface, 0);

	Mat thrLabs(Size(roiEnface.width, roiEnface.height), CV_8U, enfaceMask);

	

	Moments m = moments(thrLabs, true);
	Point p1(m.m10 / m.m00, m.m01 / m.m00);

	double x= p1.x;
	centerFrame[0] = (int)x;

	Ipp8u* maskLabels = (Ipp8u*)ippsMalloc_8u(roiEnface.width*roiEnface.height);
	Mat msklbs(Size(roiEnface.width, roiEnface.height), CV_8U, maskLabels);
	Ipp8u* largest = (Ipp8u*)ippsMalloc_8u(roiEnface.width*roiEnface.height);
	Ipp8u* secondLargest = (Ipp8u*)ippsMalloc_8u(roiEnface.width*roiEnface.height);
	status = ippiCopy_8u_C1R(enfaceMask, roiEnface.width, maskLabels, roiEnface.width, roiEnface);
	Ipp8u* morphed = morphCloseAndOpen(maskLabels, roiEnface);
	status = ippiCopy_8u_C1R(morphed, roiEnface.width, maskLabels, roiEnface.width, roiEnface);
	int pBufSize,pNumber;
	status = ippiLabelMarkersGetBufferSize_8u_C1R(roiEnface, &pBufSize);
	Ipp8u* pBuffer = (Ipp8u*)ippsMalloc_8u(pBufSize);
	status = ippiLabelMarkers_8u_C1IR(maskLabels, roiEnface.width, roiEnface, 1, 100, ippiNormInf, &pNumber, pBuffer);
	int counts;

	//Find largest connected region
	int maxCount = 0;
	int max2Count = 0;
	int maxIdx = 0;
	int max2nd = 101;
	for (int i = 1; i <= pNumber; i++) {
		status = ippiCountInRange_8u_C1R(maskLabels, roiEnface.width, roiEnface, &counts, i, i);
		if (counts > max2Count) {
			
			if (counts > maxCount) {
				maxCount = counts;
				max2nd = maxIdx;
				maxIdx = i;
			}
			else {
				max2nd = i;
				max2Count = counts;
			}
		}
	}

	status = ippiCompareC_8u_C1R(maskLabels, roiEnface.width, maxIdx, largest, roiEnface.width, roiEnface, ippCmpEq);
	status = ippiCompareC_8u_C1R(maskLabels, roiEnface.width, max2nd, secondLargest, roiEnface.width, roiEnface, ippCmpEq);
	status = ippiDivC_8u_C1IRSfs(255, largest, roiEnface.width, roiEnface, 0);
	status = ippiDivC_8u_C1IRSfs(255, secondLargest, roiEnface.width, roiEnface, 0);

	status = ippiAdd_8u_C1RSfs(largest, roiEnface.width, secondLargest, roiEnface.width, enfaceMask,roiEnface.width,roiEnface, 0);


	labels.release();
	centers.release();

	ippsFree(kmThresh1);
	ippsFree(kmThresh2);
	ippsFree(km32);
	thrLabs.release();
	ippsFree(maskLabels);
	ippsFree(pBuffer);
	ippsFree(largest);
	ippsFree(secondLargest);
	ippsFree(morphed);
}

void createEpithMap(Ipp8u* TS,Ipp8u* BM, Ipp32f* epithMap, IppiSize roi, IppiSize roiEnface, int currentFrame) {
	float rBM, rTS, diff;
	IppStatus status;
	Ipp8u pMaxBM,pMaxTS;
	int rIndx, zIndx;

	Mat bm(Size(roi.width, roi.height), CV_8U, BM);
	Mat ts(Size(roi.width, roi.height), CV_8U, TS);
	Mat epith(Size(roiEnface.width, roiEnface.height), CV_32F, epithMap);
	for (int j = 0; j < roi.height; j++) {
		status = ippiMaxIndx_8u_C1R(&BM[j*roi.width], roi.width, { roi.width,1 }, &pMaxBM, &rIndx, &zIndx);
		rBM = -1.0; rTS = -1.0;
		//if there is BM in this A-line, look for TS in the same A-line
		if (pMaxBM == 2) {
			rBM = (float)rIndx;
			status = ippiMaxIndx_8u_C1R(&TS[j*roi.width], roi.width, { roi.width,1 }, &pMaxTS, &rIndx, &zIndx);

			if (pMaxTS == 1) {
				rTS = (float)rIndx;
			}
		}
		diff = rBM - rTS;

		//If both TS and BM exist in A-line, add difference in r-dimension to the epithelium map
		if (rTS > 0.0 && rBM > 0.0 && diff > 0.0) {
			status = ippiSet_32f_C1R(diff, &epithMap[j*roiEnface.width + currentFrame], roiEnface.width * 4, { 1,1 });
		}
		
	}
}
