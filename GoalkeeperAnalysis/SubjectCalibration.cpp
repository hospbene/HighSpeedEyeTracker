#include "SubjectCalibration.h"
#include <cameraCalibration.h>

using namespace std;

CameraCalibration cameraCalibration;


SubjectCalibration::SubjectCalibration()
{

}

void SubjectCalibration::setParameters(double focalLength2, Point2d principalPoint, Mat rotationMatrix2, Point3d translationVector2, double principalPoint_pixels_c2, double principalPoint_pixels_r2, double pixelPitch_mm_c2, double pixelPitch_mm_r2)
{
	focalLength = -focalLength2;
	nodalPointO.x = principalPoint.x;
	nodalPointO.y = principalPoint.y;
	nodalPointO.z = 0;

	// left light source
	l1.x = -230.0;
	l1.y = 0.0;
	l1.z = focalLength*(-1);

	// mid light source
	l2.x = 0.0;
	l2.y = 0.0;
	l2.z = focalLength*(-1);

	// right light source
	l3.x = 230.0;
	l3.y = 0.0;
	l3.z = focalLength*(-1);

	rotationMatrix = rotationMatrix2;
	translationVector = translationVector2;
	principalPoint_pixels_c = principalPoint_pixels_c2;
	principalPoint_pixels_r = principalPoint_pixels_c2;
	pixelPitch_mm_c = pixelPitch_mm_c2;
	pixelPitch_mm_r = pixelPitch_mm_r2;
}


// calculate Cross product of glint1, glint2 glint 3 and pupil for each eye and save
// as combination according name of calibration point as
// eyeVectorCombination (eyeVecLeft, eyeVecRight, calibrationpoint name and screen point)
// in allEyeVectos.
void SubjectCalibration::computePupilGlintConnections(Point2d eyeLeft, Point2d eyeRight, Point2d screen, QString name)
{

	eyeVectorScreenCombinations combi = eyeVectorScreenCombinations();
	combi.eyeVec_left = eyeLeft;
	combi.eyeVec_right = eyeRight;
	combi.name = name;
	combi.screen = screen;
	allEyeVectors.push_back(combi);
}

void SubjectCalibration::printCombi()
{
	for (int i = 0; i < allEyeVectors.size(); i++)
	{
		qDebug() << "For" << allEyeVectors[i].name << "Screen:" << allEyeVectors[i].screen.x << allEyeVectors[i].screen.y << "EyeVector Left:" << allEyeVectors[i].eyeVec_left.x << allEyeVectors[i].eyeVec_left.y << "EyeVector Right:" << allEyeVectors[i].eyeVec_right.x << allEyeVectors[i].eyeVec_right.y;
	}
}


// save all screen Values as X and Y in screenMat
void SubjectCalibration::createScreenMat()
{
	/*
	screenMat = (cv::Mat_<double>(9, 2);


	for (int i = 0; i < allEyeVectors.size(); i++)
	{
		double t[2] = { allEyeVectors[i].screen.x,  allEyeVectors[i].screen.y };
		screenMat.push_back(Mat(1, 9, CV_64F, t));		
	}*/


	screenMat = (cv::Mat_<double>(9, 2) << allEyeVectors[0].screen.x, allEyeVectors[0].screen.y,
											allEyeVectors[1].screen.x, allEyeVectors[1].screen.y,
											allEyeVectors[2].screen.x, allEyeVectors[2].screen.y,
											allEyeVectors[3].screen.x, allEyeVectors[3].screen.y,
											allEyeVectors[4].screen.x, allEyeVectors[4].screen.y,
											allEyeVectors[5].screen.x, allEyeVectors[5].screen.y,
											allEyeVectors[6].screen.x, allEyeVectors[6].screen.y,
											allEyeVectors[7].screen.x, allEyeVectors[7].screen.y,
											allEyeVectors[8].screen.x, allEyeVectors[8].screen.y);
	
}


Mat SubjectCalibration::createLeftEyeMat()
{

	//leftEyeMat = Mat::ones(allEyeVectors.size(), unknowns, CV_64F);
	// add all X Y XY combinations to mat
/*
	double x;
	double y;
	double xy;
	double xx;
	double yy;
	double xyy;
	double yxx;
	double xxyy;
	Mat t;
	lefteyeMat = cv::Mat(9,9, CV_64F);

	for (int i = 0; i < allEyeVectors.size(); i++)
	{
		x = allEyeVectors[i].eyeVec_left.x;
		y = allEyeVectors[i].eyeVec_left.y;
		xy =  x*y;
		xx = x *x;
		yy = y * y;
		xyy = x * yy;
		yxx =y * xx;
		xxyy = xx*yy;

		double t[9] = { 1, x, y, xy, xx, yy, xyy, yxx, xxyy};
		lefteyeMat.push_back(Mat(1, 9, CV_64F, t));
	}

	righteyeMat = cv::Mat(9, 9, CV_64F);

	for (int i = 0; i < allEyeVectors.size(); i++)
	{
		x = allEyeVectors[i].eyeVec_right.x;
		y = allEyeVectors[i].eyeVec_right.y;
		xy = x*y;
		xx = x *x;
		yy = y * y;
		xyy = x * yy;
		yxx = y * xx;
		xxyy = xx*yy;

		double t[9] = { 1, x, y, xy, xx, yy, xyy, yxx, xxyy };
		righteyeMat.push_back(Mat(1, 9, CV_64F, t));
	}*/
											//				x							y								xy																	xx															 yy,																					 xyy,																										yxx,																	xxyy
	
		lefteyeMat = (cv::Mat_<double>(9, 9) << 1, allEyeVectors[0].eyeVec_left.x, allEyeVectors[0].eyeVec_left.y, allEyeVectors[0].eyeVec_left.x * allEyeVectors[0].eyeVec_left.y, allEyeVectors[0].eyeVec_left.x * allEyeVectors[0].eyeVec_left.x, allEyeVectors[0].eyeVec_left.y * allEyeVectors[0].eyeVec_left.y, allEyeVectors[0].eyeVec_left.x * allEyeVectors[0].eyeVec_left.y * allEyeVectors[0].eyeVec_left.y, allEyeVectors[0].eyeVec_left.y * allEyeVectors[0].eyeVec_left.x * allEyeVectors[0].eyeVec_left.x, allEyeVectors[0].eyeVec_left.x * allEyeVectors[0].eyeVec_left.x * allEyeVectors[0].eyeVec_left.y * allEyeVectors[0].eyeVec_left.y,
												1, allEyeVectors[1].eyeVec_left.x, allEyeVectors[1].eyeVec_left.y, allEyeVectors[1].eyeVec_left.x * allEyeVectors[1].eyeVec_left.y, allEyeVectors[1].eyeVec_left.x * allEyeVectors[1].eyeVec_left.y, allEyeVectors[1].eyeVec_left.y * allEyeVectors[1].eyeVec_left.y, allEyeVectors[1].eyeVec_left.x * allEyeVectors[1].eyeVec_left.y * allEyeVectors[1].eyeVec_left.y, allEyeVectors[1].eyeVec_left.y * allEyeVectors[1].eyeVec_left.x * allEyeVectors[1].eyeVec_left.x, allEyeVectors[1].eyeVec_left.x * allEyeVectors[1].eyeVec_left.x * allEyeVectors[1].eyeVec_left.y * allEyeVectors[1].eyeVec_left.y,
												1, allEyeVectors[2].eyeVec_left.x, allEyeVectors[2].eyeVec_left.y, allEyeVectors[2].eyeVec_left.x * allEyeVectors[2].eyeVec_left.y, allEyeVectors[2].eyeVec_left.x * allEyeVectors[2].eyeVec_left.y, allEyeVectors[2].eyeVec_left.y * allEyeVectors[2].eyeVec_left.y, allEyeVectors[2].eyeVec_left.x * allEyeVectors[2].eyeVec_left.y * allEyeVectors[2].eyeVec_left.y, allEyeVectors[2].eyeVec_left.y * allEyeVectors[2].eyeVec_left.x * allEyeVectors[2].eyeVec_left.x, allEyeVectors[2].eyeVec_left.x * allEyeVectors[2].eyeVec_left.x * allEyeVectors[2].eyeVec_left.y * allEyeVectors[2].eyeVec_left.y,
												1, allEyeVectors[3].eyeVec_left.x, allEyeVectors[3].eyeVec_left.y, allEyeVectors[3].eyeVec_left.x * allEyeVectors[3].eyeVec_left.y, allEyeVectors[3].eyeVec_left.x * allEyeVectors[3].eyeVec_left.y, allEyeVectors[3].eyeVec_left.y * allEyeVectors[3].eyeVec_left.y, allEyeVectors[3].eyeVec_left.x * allEyeVectors[3].eyeVec_left.y * allEyeVectors[3].eyeVec_left.y, allEyeVectors[3].eyeVec_left.y * allEyeVectors[3].eyeVec_left.x * allEyeVectors[3].eyeVec_left.x, allEyeVectors[3].eyeVec_left.x * allEyeVectors[3].eyeVec_left.x * allEyeVectors[3].eyeVec_left.y * allEyeVectors[3].eyeVec_left.y,
												1, allEyeVectors[4].eyeVec_left.x, allEyeVectors[4].eyeVec_left.y, allEyeVectors[4].eyeVec_left.x * allEyeVectors[4].eyeVec_left.y, allEyeVectors[4].eyeVec_left.x * allEyeVectors[4].eyeVec_left.y, allEyeVectors[4].eyeVec_left.y * allEyeVectors[4].eyeVec_left.y, allEyeVectors[4].eyeVec_left.x * allEyeVectors[4].eyeVec_left.y * allEyeVectors[4].eyeVec_left.y, allEyeVectors[4].eyeVec_left.y * allEyeVectors[4].eyeVec_left.x * allEyeVectors[4].eyeVec_left.x, allEyeVectors[4].eyeVec_left.x * allEyeVectors[4].eyeVec_left.x * allEyeVectors[4].eyeVec_left.y * allEyeVectors[4].eyeVec_left.y,
												1, allEyeVectors[5].eyeVec_left.x, allEyeVectors[5].eyeVec_left.y, allEyeVectors[5].eyeVec_left.x * allEyeVectors[5].eyeVec_left.y, allEyeVectors[5].eyeVec_left.x * allEyeVectors[5].eyeVec_left.y, allEyeVectors[5].eyeVec_left.y * allEyeVectors[5].eyeVec_left.y, allEyeVectors[5].eyeVec_left.x * allEyeVectors[5].eyeVec_left.y * allEyeVectors[5].eyeVec_left.y, allEyeVectors[5].eyeVec_left.y * allEyeVectors[5].eyeVec_left.x * allEyeVectors[5].eyeVec_left.x, allEyeVectors[5].eyeVec_left.x * allEyeVectors[5].eyeVec_left.x * allEyeVectors[5].eyeVec_left.y * allEyeVectors[5].eyeVec_left.y,
												1, allEyeVectors[6].eyeVec_left.x, allEyeVectors[6].eyeVec_left.y, allEyeVectors[6].eyeVec_left.x * allEyeVectors[6].eyeVec_left.y, allEyeVectors[6].eyeVec_left.x * allEyeVectors[6].eyeVec_left.y, allEyeVectors[6].eyeVec_left.y * allEyeVectors[6].eyeVec_left.y, allEyeVectors[6].eyeVec_left.x * allEyeVectors[6].eyeVec_left.y * allEyeVectors[6].eyeVec_left.y, allEyeVectors[6].eyeVec_left.y * allEyeVectors[6].eyeVec_left.x * allEyeVectors[6].eyeVec_left.x, allEyeVectors[6].eyeVec_left.x * allEyeVectors[6].eyeVec_left.x * allEyeVectors[6].eyeVec_left.y * allEyeVectors[6].eyeVec_left.y,
												1, allEyeVectors[7].eyeVec_left.x, allEyeVectors[7].eyeVec_left.y, allEyeVectors[7].eyeVec_left.x * allEyeVectors[7].eyeVec_left.y, allEyeVectors[7].eyeVec_left.x * allEyeVectors[7].eyeVec_left.y, allEyeVectors[7].eyeVec_left.y * allEyeVectors[7].eyeVec_left.y, allEyeVectors[7].eyeVec_left.x * allEyeVectors[7].eyeVec_left.y * allEyeVectors[7].eyeVec_left.y, allEyeVectors[7].eyeVec_left.y * allEyeVectors[7].eyeVec_left.x * allEyeVectors[7].eyeVec_left.x, allEyeVectors[7].eyeVec_left.x * allEyeVectors[7].eyeVec_left.x * allEyeVectors[7].eyeVec_left.y * allEyeVectors[7].eyeVec_left.y,
												1, allEyeVectors[8].eyeVec_left.x, allEyeVectors[8].eyeVec_left.y, allEyeVectors[8].eyeVec_left.x * allEyeVectors[8].eyeVec_left.y, allEyeVectors[8].eyeVec_left.x * allEyeVectors[8].eyeVec_left.y, allEyeVectors[8].eyeVec_left.y * allEyeVectors[8].eyeVec_left.y, allEyeVectors[8].eyeVec_left.x * allEyeVectors[8].eyeVec_left.y * allEyeVectors[8].eyeVec_left.y, allEyeVectors[8].eyeVec_left.y * allEyeVectors[8].eyeVec_left.x * allEyeVectors[8].eyeVec_left.x, allEyeVectors[8].eyeVec_left.x * allEyeVectors[8].eyeVec_left.x * allEyeVectors[8].eyeVec_left.y * allEyeVectors[8].eyeVec_left.y);
							
		return lefteyeMat;
 }



Mat SubjectCalibration::createRightEyeMat()
{

		//righteyeMat = Mat::ones(allEyeVectors.size(), unknowns, CV_64F);
	// add all X Y XY combinations to mat

	righteyeMat = (cv::Mat_<double>(9, 9) << 1, allEyeVectors[0].eyeVec_right.x, allEyeVectors[0].eyeVec_right.y, allEyeVectors[0].eyeVec_right.x * allEyeVectors[0].eyeVec_right.y, allEyeVectors[0].eyeVec_right.x * allEyeVectors[0].eyeVec_right.x, allEyeVectors[0].eyeVec_right.y * allEyeVectors[0].eyeVec_right.y, allEyeVectors[0].eyeVec_right.x * allEyeVectors[0].eyeVec_right.y * allEyeVectors[0].eyeVec_right.y, allEyeVectors[0].eyeVec_right.y * allEyeVectors[0].eyeVec_right.x * allEyeVectors[0].eyeVec_right.x, allEyeVectors[0].eyeVec_right.x * allEyeVectors[0].eyeVec_right.x * allEyeVectors[0].eyeVec_right.y * allEyeVectors[0].eyeVec_right.y,
		1, allEyeVectors[1].eyeVec_right.x, allEyeVectors[1].eyeVec_right.y, allEyeVectors[1].eyeVec_right.x * allEyeVectors[1].eyeVec_right.y, allEyeVectors[1].eyeVec_right.x * allEyeVectors[1].eyeVec_right.y, allEyeVectors[1].eyeVec_right.y * allEyeVectors[1].eyeVec_right.y, allEyeVectors[1].eyeVec_right.x * allEyeVectors[1].eyeVec_right.y * allEyeVectors[1].eyeVec_right.y, allEyeVectors[1].eyeVec_right.y * allEyeVectors[1].eyeVec_right.x * allEyeVectors[1].eyeVec_right.x, allEyeVectors[1].eyeVec_right.x * allEyeVectors[1].eyeVec_right.x * allEyeVectors[1].eyeVec_right.y * allEyeVectors[1].eyeVec_right.y,
		1, allEyeVectors[2].eyeVec_right.x, allEyeVectors[2].eyeVec_right.y, allEyeVectors[2].eyeVec_right.x * allEyeVectors[2].eyeVec_right.y, allEyeVectors[2].eyeVec_right.x * allEyeVectors[2].eyeVec_right.y, allEyeVectors[2].eyeVec_right.y * allEyeVectors[2].eyeVec_right.y, allEyeVectors[2].eyeVec_right.x * allEyeVectors[2].eyeVec_right.y * allEyeVectors[2].eyeVec_right.y, allEyeVectors[2].eyeVec_right.y * allEyeVectors[2].eyeVec_right.x * allEyeVectors[2].eyeVec_right.x, allEyeVectors[2].eyeVec_right.x * allEyeVectors[2].eyeVec_right.x * allEyeVectors[2].eyeVec_right.y * allEyeVectors[2].eyeVec_right.y,
		1, allEyeVectors[3].eyeVec_right.x, allEyeVectors[3].eyeVec_right.y, allEyeVectors[3].eyeVec_right.x * allEyeVectors[3].eyeVec_right.y, allEyeVectors[3].eyeVec_right.x * allEyeVectors[3].eyeVec_right.y, allEyeVectors[3].eyeVec_right.y * allEyeVectors[3].eyeVec_right.y, allEyeVectors[3].eyeVec_right.x * allEyeVectors[3].eyeVec_right.y * allEyeVectors[3].eyeVec_right.y, allEyeVectors[3].eyeVec_right.y * allEyeVectors[3].eyeVec_right.x * allEyeVectors[3].eyeVec_right.x, allEyeVectors[3].eyeVec_right.x * allEyeVectors[3].eyeVec_right.x * allEyeVectors[3].eyeVec_right.y * allEyeVectors[3].eyeVec_right.y,
		1, allEyeVectors[4].eyeVec_right.x, allEyeVectors[4].eyeVec_right.y, allEyeVectors[4].eyeVec_right.x * allEyeVectors[4].eyeVec_right.y, allEyeVectors[4].eyeVec_right.x * allEyeVectors[4].eyeVec_right.y, allEyeVectors[4].eyeVec_right.y * allEyeVectors[4].eyeVec_right.y, allEyeVectors[4].eyeVec_right.x * allEyeVectors[4].eyeVec_right.y * allEyeVectors[4].eyeVec_right.y, allEyeVectors[4].eyeVec_right.y * allEyeVectors[4].eyeVec_right.x * allEyeVectors[4].eyeVec_right.x, allEyeVectors[4].eyeVec_right.x * allEyeVectors[4].eyeVec_right.x * allEyeVectors[4].eyeVec_right.y * allEyeVectors[4].eyeVec_right.y,
		1, allEyeVectors[5].eyeVec_right.x, allEyeVectors[5].eyeVec_right.y, allEyeVectors[5].eyeVec_right.x * allEyeVectors[5].eyeVec_right.y, allEyeVectors[5].eyeVec_right.x * allEyeVectors[5].eyeVec_right.y, allEyeVectors[5].eyeVec_right.y * allEyeVectors[5].eyeVec_right.y, allEyeVectors[5].eyeVec_right.x * allEyeVectors[5].eyeVec_right.y * allEyeVectors[5].eyeVec_right.y, allEyeVectors[5].eyeVec_right.y * allEyeVectors[5].eyeVec_right.x * allEyeVectors[5].eyeVec_right.x, allEyeVectors[5].eyeVec_right.x * allEyeVectors[5].eyeVec_right.x * allEyeVectors[5].eyeVec_right.y * allEyeVectors[5].eyeVec_right.y,
		1, allEyeVectors[6].eyeVec_right.x, allEyeVectors[6].eyeVec_right.y, allEyeVectors[6].eyeVec_right.x * allEyeVectors[6].eyeVec_right.y, allEyeVectors[6].eyeVec_right.x * allEyeVectors[6].eyeVec_right.y, allEyeVectors[6].eyeVec_right.y * allEyeVectors[6].eyeVec_right.y, allEyeVectors[6].eyeVec_right.x * allEyeVectors[6].eyeVec_right.y * allEyeVectors[6].eyeVec_right.y, allEyeVectors[6].eyeVec_right.y * allEyeVectors[6].eyeVec_right.x * allEyeVectors[6].eyeVec_right.x, allEyeVectors[6].eyeVec_right.x * allEyeVectors[6].eyeVec_right.x * allEyeVectors[6].eyeVec_right.y * allEyeVectors[6].eyeVec_right.y,
		1, allEyeVectors[7].eyeVec_right.x, allEyeVectors[7].eyeVec_right.y, allEyeVectors[7].eyeVec_right.x * allEyeVectors[7].eyeVec_right.y, allEyeVectors[7].eyeVec_right.x * allEyeVectors[7].eyeVec_right.y, allEyeVectors[7].eyeVec_right.y * allEyeVectors[7].eyeVec_right.y, allEyeVectors[7].eyeVec_right.x * allEyeVectors[7].eyeVec_right.y * allEyeVectors[7].eyeVec_right.y, allEyeVectors[7].eyeVec_right.y * allEyeVectors[7].eyeVec_right.x * allEyeVectors[7].eyeVec_right.x, allEyeVectors[7].eyeVec_right.x * allEyeVectors[7].eyeVec_right.x * allEyeVectors[7].eyeVec_right.y * allEyeVectors[7].eyeVec_right.y,
		1, allEyeVectors[8].eyeVec_right.x, allEyeVectors[8].eyeVec_right.y, allEyeVectors[8].eyeVec_right.x * allEyeVectors[8].eyeVec_right.y, allEyeVectors[8].eyeVec_right.x * allEyeVectors[8].eyeVec_right.y, allEyeVectors[8].eyeVec_right.y * allEyeVectors[8].eyeVec_right.y, allEyeVectors[8].eyeVec_right.x * allEyeVectors[8].eyeVec_right.y * allEyeVectors[8].eyeVec_right.y, allEyeVectors[8].eyeVec_right.y * allEyeVectors[8].eyeVec_right.x * allEyeVectors[8].eyeVec_right.x, allEyeVectors[8].eyeVec_right.x * allEyeVectors[8].eyeVec_right.x * allEyeVectors[8].eyeVec_right.y * allEyeVectors[8].eyeVec_right.y);

	return righteyeMat;
}


vector<Mat_<double>> SubjectCalibration::calculateTransformationMatrix()
{

	/* Usually A*x = b can be used for this case.
	ScreenMatrix = EyeVectorMatrix * coefficients
	ScreenMatrix contains all Calibrationpoints (9)
	EyeVectorMatrix Contains for each row a polynom
	e.g. X Y XY	
	Coefficientmatrix contains the coefficients that must be calculated to get from EyeVectors to Screenpoints

	XScreen YScreen					x			y				XY								a b

	Xscreen1 Yscreen1		1 EyeVectorX1 EyeVectorY1  EyeVectorX1 * EyeVectorY1				a0 b0
	   .        .			. 		.			.					.							a1 b1
	   .        .		=	.		.			.					.					*		a2 b2
	   .        .			.		.			.					.							a3 b3
	Xscreen9 Yscreen9		1 EyeVectorX9 EyeVectorY9  EyeVectorX9 * EyeVectorY9
	\     Mat A		/		\			Polynom coefficients					/		   \ polynom unknowns /

	*/

	bool result = true;
	result = solve(lefteyeMat, screenMat, ab_left, DECOMP_SVD);
	qDebug() << "Solve right: " << result;

	result = solve(righteyeMat, screenMat, ab_right, DECOMP_SVD);
	qDebug() << "Solve left: " << result;


	cout << "Screen: " << endl << screenMat << endl;
	cout << "LeftEyeMat: " << endl << lefteyeMat << endl;
	cout << "rightEyeMat: " << endl << righteyeMat << endl;
	cout << "ab_left: " << endl << ab_left << endl;
	cout << "ab_right: " << endl << ab_right << endl;

	Mat m = lefteyeMat * ab_left;
	qDebug() << endl << "ScreenMat Calculated:" << endl;
	cout << m << endl << endl;
	vector<Mat_<double>> asdf;
	asdf.push_back(ab_left);
	asdf.push_back(ab_right);

	return  asdf;

}




Point2d SubjectCalibration::translateGaze(Point2d eyeVec, bool left)
{

	qDebug() << "EyeVec :" << eyeVec.x << eyeVec.y << endl;
		
	Point2d gaze(0,0);
	 if (left)
	 {
		 //			1,						x,										y,									 xy,												 xx,											 yy,												xyy,													yxx,														xxyy
		 gaze.x = ab_left.at<double>(0,0) + (ab_left.at<double>(1,0) * eyeVec.x) + (ab_left.at<double>(2,0)* eyeVec.y) + (ab_left.at<double>(3,0) * (eyeVec.x*eyeVec.y)) + (ab_left.at<double>(4, 0) * (eyeVec.x*eyeVec.x)) + (ab_left.at<double>(5, 0) * (eyeVec.y*eyeVec.y)) + (ab_left.at<double>(6, 0) * (eyeVec.x*eyeVec.y*eyeVec.y)) + (ab_left.at<double>(7, 0) * (eyeVec.y*eyeVec.x*eyeVec.x)) + (ab_left.at<double>(8, 0) * (eyeVec.x*eyeVec.x*eyeVec.x*eyeVec.y*eyeVec.y));
		 gaze.y = ab_left.at<double>(1, 1) + (ab_left.at<double>(1, 1) * eyeVec.x) + (ab_left.at<double>(2, 1)* eyeVec.y) + (ab_left.at<double>(3, 1) * (eyeVec.x*eyeVec.y)) + (ab_left.at<double>(4, 1) * (eyeVec.x*eyeVec.x)) + (ab_left.at<double>(5, 1) * (eyeVec.y*eyeVec.y)) + (ab_left.at<double>(6, 1) * (eyeVec.x*eyeVec.y*eyeVec.y)) + (ab_left.at<double>(7, 1) * (eyeVec.y*eyeVec.x*eyeVec.x)) + (ab_left.at<double>(8, 1) * (eyeVec.x*eyeVec.x*eyeVec.x*eyeVec.y*eyeVec.y));

	 }
	 else
	 {
		 gaze.x = ab_right.at<double>(1, 1) + (ab_right.at<double>(1,0) * eyeVec.x) + (ab_right.at<double>(2,0) * eyeVec.y) + (ab_right.at<double>(3,0) * (eyeVec.x*eyeVec.y));
		 gaze.y = ab_right.at<double>(0, 1) + (ab_right.at<double>(1,1) * eyeVec.x) + (ab_right.at<double>(2,1) * eyeVec.y) + (ab_right.at<double>(3,1) * (eyeVec.x*eyeVec.y));
	 }

	 qDebug() << " Screen:" << gaze.x << gaze.y << endl;
 	 return gaze;
}











// glints müssen übergeben werden
void SubjectCalibration::computeCenterOfCornea(std::vector<cv::Point3d> glints)
{
	// left eye

	// corneal center
	Point3d center;

	// 1. Create Points from glint list
	Point3d u1(glints[0].x, glints[0].y, glints[0].z);	// u1
	Point3d u2(glints[1].x, glints[1].y, glints[1].z);	// u2
	Point3d u3(glints[2].x, glints[2].y, glints[2].z);	// u3

														// Formula (14.1) for each glint
														// TODO: depending on numver of found glints
														// Generate Lightsource - Glint combination vectors
	Point3d m1 = (l1 - nodalPointO).cross(u1 - nodalPointO);
	Point3d m2 = (l2 - nodalPointO).cross(u2 - nodalPointO);
	Point3d m3 = (l3 - nodalPointO).cross(u3 - nodalPointO);


	qDebug() << "L1: " << l1.x << l1.y << l1.z;
	qDebug() << "NodalPointO: " << nodalPointO.x << nodalPointO.y << nodalPointO.z;

	// (14.2) build Matrix

	/*y1*cx*/	/*y2*cy*/	/*y3*cz*/	 /* y4 result*/
	Mat M = (cv::Mat_<double>(3, 4) << m1.x, m1.y, m1.z, (m1.x*nodalPointO.x + m1.y*nodalPointO.y + m1.z*nodalPointO.z),
		m2.x, m2.y, m2.z, (m2.x*nodalPointO.x + m2.y*nodalPointO.y + m2.z*nodalPointO.z),
		m3.x, m3.y, m3.z, (m3.x*nodalPointO.x + m3.y*nodalPointO.y + m3.z*nodalPointO.z));

	cout << M << endl << endl;

	// Number of rows
	int n = 3;
	vector<double> line(n + 1, 0);
	vector< vector<double> > A(n, line);

	// Read input data
	A[0][0] = m1.x;
	A[0][1] = m1.y;
	A[0][2] = m1.z;
	A[0][3] = (m1.x*nodalPointO.x + m1.y*nodalPointO.y + m1.z*nodalPointO.z);

	A[1][0] = m2.x;
	A[1][1] = m2.y;
	A[1][2] = m2.z;
	A[1][3] = (m2.x*nodalPointO.x + m2.y*nodalPointO.y + m2.z*nodalPointO.z);

	A[2][0] = m3.x;
	A[2][1] = m3.y;
	A[2][2] = m3.z;
	A[2][3] = (m3.x*nodalPointO.x + m3.y*nodalPointO.y + m3.z*nodalPointO.z);


	// Compute A*x = b
	// solution is c = center of cornea


	// Print input
	print(A);

	// Calculate solution
	vector<double> x(n);
	x = gauss(A);

	// Print result
	cout << "Result:\t";
	for (int i = 0; i<n; i++)
	{
		cout << x[i] << " ";
	}
	cout << endl;





	// right eye

}


void SubjectCalibration::print(vector< vector<double> > A)
{
	int n = A.size();
	for (int i = 0; i<n; i++)
	{
		for (int j = 0; j<n + 1; j++)
		{
			cout << A[i][j] << "\t";
			if (j == n - 1)
			{
				cout << "| ";
			}
		}
		cout << "\n";
	}
	cout << endl;
}









vector<double> SubjectCalibration::gauss(std::vector<std::vector<double>> A)
{
	int n = A.size();

	for (int i = 0; i<n; i++)
	{
		// Search for maximum in this column
		double maxEl = abs(A[i][i]);
		int maxRow = i;
		for (int k = i + 1; k<n; k++)
		{
			if (abs(A[k][i]) > maxEl)
			{
				maxEl = abs(A[k][i]);
				maxRow = k;
			}
		}

		// Swap maximum row with current row (column by column)
		for (int k = i; k<n + 1; k++)
		{
			double tmp = A[maxRow][k];
			A[maxRow][k] = A[i][k];
			A[i][k] = tmp;
		}

		// Make all rows below this one 0 in current column
		for (int k = i + 1; k<n; k++)
		{
			double c = -A[k][i] / A[i][i];
			for (int j = i; j<n + 1; j++)
			{
				if (i == j)
				{
					A[k][j] = 0;
				}
				else
				{
					A[k][j] += c * A[i][j];
				}
			}
		}
	}

	// Solve equation Ax=b for an upper triangular matrix A
	vector<double> x(n);
	for (int i = n - 1; i >= 0; i--)
	{
		x[i] = A[i][n] / A[i][i];
		for (int k = i - 1; k >= 0; k--)
		{
			A[k][n] -= A[k][i] * x[i];
		}
	}
	return x;
}

Point3d SubjectCalibration::calcMatMulVec(Mat rotationMatrix, Point3d ccsPoint)
{
	Point3d res;
	res.x = (rotationMatrix.at<double>(0, 0) * ccsPoint.x) + (rotationMatrix.at<double>(0, 1) * ccsPoint.y) + (rotationMatrix.at<double>(0, 2) * ccsPoint.z);
	res.y = (rotationMatrix.at<double>(1, 0) * ccsPoint.x) + (rotationMatrix.at<double>(1, 1) * ccsPoint.y) + (rotationMatrix.at<double>(1, 2) * ccsPoint.z);
	res.z = (rotationMatrix.at<double>(2, 0) * ccsPoint.x) + (rotationMatrix.at<double>(2, 1) * ccsPoint.y) + (rotationMatrix.at<double>(2, 2) * ccsPoint.z);

	return res;
}

cv::Mat SubjectCalibration::generateMatrixMFromVectors(Vec3d m1, Vec3d m2, Vec3d m3)
{

	Mat M(3, 3, CV_8UC3);
	M.at<double>(0, 0) = m1[0];
	M.at<double>(1, 0) = m1[1];
	M.at<double>(2, 0) = m1[2];

	M.at<double>(0, 1) = m2[0];
	M.at<double>(1, 1) = m2[1];
	M.at<double>(2, 1) = m2[2];

	M.at<double>(0, 2) = m3[0];
	M.at<double>(1, 2) = m3[1];
	M.at<double>(2, 2) = m3[2];

	return M;
}

cv::Point3d SubjectCalibration::transformICStoWCS(Point2d icsPoint)
{
	/*
	To get transformation from Image coordinate system (ICS)
	to the world coordinate system (WCS) it is convenient to define a camera coordinate system (CCS)

	CCS is a right handed 3D Cartesian coordinate system where:
	x-axis = parallel to rows of image sensor
	y-axis = parallel to the columns of the image sensor
	z-axis = perpendicular/orthogonal to the plane of image sensor
	and coincident (same) with optic axis of camera.
	origin = coincident with nodal point of camera o


	ICS is a right handed 2D cartesion coordinate system where:
	coordinates are measured in pixels.

	r = row, in pixels
	c = column, in pixels*/

	/*===========================================================================
	how to  transform ICS to CCS:
	from row r and column c in pixels in ICS
	to length coordinates [X Y Z ]^T

	[X Y Z ]^T = [pixelpitch_c*(c-c_center), pixelpitch_r* (r- r_center), -lambda]

	c_center = column coordinate in pixels from intersection of optic axis and image plane (principal point/image center)
	r_center = row coordinate in pixels from intersection of optic axis and image plane (principal point/image center)

	pixelpitch_c = distance in units of length between adjacent pixels across columns.
	pixelpitch_r = distance in units of length between adjacent pixels across rows.

	lambda = distance between nodal point and image plane (for pinhole model this equals the focal length)
	for camera which has to be focused, lambda is related to focal length by gaussian lens formula:
	1/ (distance object - lens) + 1/lambda = 1 /focal length


	Intrinsic parameters:
	-> lambda => CameraCalibration.focalLength;
	-> c_center => CameraCalibration.principalPoint but in mm not in pixel;
	-> r_center => CameraCalibration.principalPoint but in mm not in pixel;
	-> pixelpitch_c
	-> and pixelpitch_r

	From Definition of CCS: all points in image plane have z = -lambda*/

	// From ICS to CCS
	Point3d ccsPoint;
	ccsPoint.x = pixelPitch_mm_c*(icsPoint.x - principalPoint_pixels_c);
	ccsPoint.y = pixelPitch_mm_r*(icsPoint.y - principalPoint_pixels_r);
	ccsPoint.z = -focalLength;


	/*===========================================================================

	how to transform from CCS to WCS:

	i_cam, j_cam, k_cam
	unit vectors,
	in the directions of the axis of CCS,
	measured in world coordinates


	point represented as [x_u, y_u, z_u]^T in CCS
	is transformed to [X_u, Y_u, Z_u]^T in WCS as:

	u = [X_u, Y_u, Z_u]^T = [i_cam, j_cam, k_cam] * [x_u, y_u, z_u]^T + o
	\      WCS      /   \        R          /   \      CCS     /


	Extrinsic parameters:
	-> position of the nodal point of the camera o (translation of the origin of the CCS with respect to the WCS)
	-> and Rotation Matrix R (the rotation of the CCS with respect to the WCS)*/


	Point3d wcsPoint = calcMatMulVec(rotationMatrix, ccsPoint);

	wcsPoint = wcsPoint + translationVector;


	qDebug() << "WCS POINT: x" << wcsPoint.x << wcsPoint.y << wcsPoint.z;

	return wcsPoint;
	/*===========================================================================


	Intrinsic parameters:
	-> lambda			=> CameraCalibration.focalLength;
	-> c_center			=> CameraCalibration.principalPoint_pixels_c
	-> r_center			=> CameraCalibration.principalPoint_pixels_r
	-> pixelpitch_c		=> CameraCalibration.pixelPitch_mm_c
	-> and pixelpitch_r	=> CameraCalibration.pixelPitch_mm_r

	Extrinsic parameters:
	-> position of the nodal point of the camera o (translation of the origin of the CCS with respect to the WCS)
	=>
	-> Rotation Matrix R (translation of origin of CCS with respect to WCS)
	*/



}
