
// OpenCV
#include <qdebug.h>
#include "opencv2/opencv.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/videoio/videoio.hpp>  // Video write
#include <opencv2/videoio/videoio_c.h>  // Video write
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <myStructs.h>

using namespace std;
using namespace cv;

class SubjectCalibration
{
public:
	// SUGGESTED VALUES FROM "GENERAL THEORY" PAPER

	double R = 7.8;				// Radius of cornea
	double K = 4.2;				// distance between Center of Pupil and Cornea
	double n1 = 1.3375;			// Effective index of refraction of cornea and aqueous humor combined, SKI

	Point3d nodalPointO;			// nodal point of the camera should be directly in front of ICS center with negative focal length as z 
	double focalLength;
	
	cv::Mat rotationMatrix;
	double principalPoint_pixels_c;
	double principalPoint_pixels_r;
	double pixelPitch_mm_c;
	double pixelPitch_mm_r;
	Point3d l1;
	Point3d l2;
	Point3d l3;
	cv::Point3d translationVector;
	int unknowns = 4;	// 4 => Polynom is x y xy



	Mat screenMat;

	Mat lefteyeMat;
	Mat_<double> righteyeMat;
	Mat_<double> lefteyeMat_transposed;
	Mat_<double> righteyeMat_transposed;
	Mat_<double> lefteyeMat_one;
	Mat_<double> righteyeMat_one;
	Mat_<double> transformationMat;
	Mat_<double> ab_left;
	Mat_<double> ab_right;



	struct eyeVectorScreenCombinations
	{
		QString name = "Point 0";
		Point2d eyeVec_left;
		Point2d eyeVec_right;
		Point2d screen;

	};

	vector<eyeVectorScreenCombinations> allEyeVectors;



	SubjectCalibration();
	void setParameters(double focalLength2, Point2d principalPoint, Mat rotationMatrix2, Point3d translationVector2, double principalPoint_pixels_c2, double principalPoint_pixels_r2, double pixelPitch_mm_c2, double pixelPitch_mm_r2);
	void computePupilGlintConnections(Point2d eyeLeft, Point2d eyeRight, Point2d screen, QString name);
	void printCombi();
	void createScreenMat();
	Mat createLeftEyeMat();
	Mat createRightEyeMat();
	vector<Mat_<double>> calculateTransformationMatrix();
	Point2d translateGaze(Point2d eyeVec, bool left);
	void computeCenterOfCornea(std::vector<cv::Point3d> glints);
	cv::Mat generateMatrixMFromVectors(cv::Vec3d m1, cv::Vec3d m2, cv::Vec3d m3);
	cv::Point3d transformICStoWCS(cv::Point2d imageGaze);
	void print(std::vector <std::vector<double> > A);
	std::vector<double> gauss(std::vector<std::vector<double>> A);
	Point3d calcMatMulVec(Mat rotationMatrix, Point3d ccsPoint);


};