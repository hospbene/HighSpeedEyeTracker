

// Qt
#include <QtWidgets/QMainWindow>
#include "ui_GoalkeeperAnalysis.h"
#include <qgraphicsscene.h>
#include <qgraphicsview.h>
#include <qpixmap.h>
#include <QDebug>
#include <qlistwidget.h>
#include <QApplication>
#include <qtextbrowser.h>
#include <qgraphicsscene.h>
#include <qgraphicsview.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <QListWidget>

// uEye
#include <uEye.h>
#include <uEye_tools.h>
#include <DynamicuEyeTools.h>

// Eigene
#include <uEyeCamera.h>
#include <cameraCalibration.h>
#include <SubjectCalibration.h>
#include <myStructs.h>


// OpenCV
#include "opencv2/opencv.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/videoio/videoio.hpp>  // Video write
#include <opencv2/videoio/videoio_c.h>  // Video write
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

// Standard / Windows
#include "WinUser.h"
#include "Timer.h"
#include <iostream>                             // Standardstream-Funktionaliät einbinden
#include <fstream>                              // ofstream und ifstream einbinde
#include <queue>
#include <chrono>
#include <exception>
#include <iostream>
#include <algorithm> 
#include <string>

// Boost
#include <boost/thread.hpp>
#include <boost/atomic.hpp>
#include <boost/filesystem.hpp>

//#include <algo.h>

#include <QDialog>
#include <QGroupBox.h>
#include <qdialogbuttonbox.h>
#include <dialog.h>
#include <fstream>
#include <memory>

#include <iostream>

class GoalkeeperAnalysis : public QMainWindow
{
	Q_OBJECT
	public:
		string uploadedVideo;
		Dialog *dialog;
		void changeGamma(int v);
		void changeGain(int v);
		void changeHWGain(int v);
		GoalkeeperAnalysis(QWidget *parent = Q_NULLPTR);
		void createUserFiles();
		~GoalkeeperAnalysis();
		QTextBrowser* logOutput;
		QTextBrowser* sensorInformation;
		QTextBrowser* cameraInformation;
		QGraphicsView* snapshotOutput;
		QListWidget* colorModeList; 

		// Output of calibration procedure
		Mat ab_left, ab_right, a_left, a_right, b_left, b_right;


		// one instance of a combination of glints and pupil center
		// a lot of these are saved in a temp structure allPointsFoundOnX
		// a temp Strcuture contains all found positions during one calibration point
		struct pupilGlintCombiInstance
		{
			boolean valid = true;
			cv::Point2f pupilLeft = 0;
			cv::Point2f pupilRight = 0;
			cv::Point2f glintLeft_1 = 0;
			cv::Point2f glintLeft_2 = 0;
			cv::Point2f glintLeft_3 = 0;
			cv::Point2f glintRight_1 = 0;
			cv::Point2f glintRight_2 = 0;
			cv::Point2f glintRight_3 = 0;


			void toString()
			{
				qDebug() << "Pl x=" << pupilLeft.x << " y=" << pupilLeft.y 
					<< "  G0l x=" << glintLeft_1.x << " y=" << glintLeft_1.y
					<< "  G1l x=" << glintLeft_2.x << " y=" << glintLeft_2.y
					<< "  G2l x=" << glintLeft_3.x << " y=" << glintLeft_3.y
					<< "  |||||| Pr x=" << pupilRight.x << " y=" << pupilRight.y
					<< "  G0r x=" << glintRight_1.x << " y=" << glintRight_1.y
					<< "  G1r x=" << glintRight_2.x << " y=" << glintRight_2.y
					<< "  G2r x=" << glintRight_3.x << " y=" << glintRight_3.y;
			}
		};
	

// GENERAL ==========================================================================================================================
		std::vector<boost::thread> v;
		std::queue <cv::Mat> framesQueue;								// Frames captured during high FPS Mode are saved in this queue by special thread
		std::queue <pupilGlintCombiInstance> gazeQueue;
		boost::mutex sharedMutex;							// mutex for writing and reading from framesQueue
		std::string currentVideo;
		Ui::GoalkeeperAnalysisClass ui;
		boolean captureRunning;
		boolean threadEndedNormally;
		std::ofstream outputfile;
		std::ofstream userVideoFile;
		std::ofstream eyeFile;

		struct brightestPointRegion
		{
			vector<Rect> regions;
			vector<Point2d> brightestPoints;
		};
	
// GLINT AND BLOB DETECTION ===================================================================================================================

			// Setup SimpleBlobDetector parameters.
			SimpleBlobDetector::Params pupilParams;

			boolean glintsFound = false;
			cv::Rect newAreaRect;
			Point leftGlint;
			Point rightGlint;
			Point leftPupil;
			Point rightPupil;
			Point leftPupilOriginal;
			Point rightPupilOriginal;
			Ptr<SimpleBlobDetector> pupilDetector;


// SAVING
			// saves start time of video,
			// end time of video
			// each frame with framenumber and ms from start
			struct videoFrame2Timestamp
			{
				__int64 startTime;
				__int64 endTime;
				std::map<double, double> frame2time;			// frame, timestamp
			} userVideoDictionary, eyeVideoDictionary;


			// saves frame number and calculated gaze point from eye Video
			std::map <int, cv::Point2d> gaze2frame;



		


// CALIBRATION ===============================================================================================================
	

			// for each calibration point 1-9 
			// mean values for glints, pupil centers
			// and screen point is saved
			struct calibrationPointCombination
			{
				QString name;
				cv::Point2d pupilLeft;
				cv::Point2d pupilRight;
				cv::Point2d glintLeft_1;
				cv::Point2d glintLeft_2;
				cv::Point2d glintLeft_3;
				cv::Point2d glintRight_1;
				cv::Point2d glintRight_2;
				cv::Point2d glintRight_3;
				cv::Point2d centerOfCrossOnScreen;


				void toString()
				{
					qDebug() << "Name: " << name << " P_Left x=" << pupilLeft.x << " y=" << pupilLeft.y << "  P_Right x=" << pupilRight.x << " y=" << pupilRight.y
						<< "  G1L x=" << glintLeft_1.x << " y=" << glintLeft_1.y
						<< "  G2L x=" << glintLeft_2.x << " y=" << glintLeft_2.y
						<< "  G3L x=" << glintLeft_3.x << " y=" << glintLeft_3.y
						<< "  G1R x=" << glintRight_1.x << " y=" << glintRight_1.y
						<< "  G2R x=" << glintRight_2.x << " y=" << glintRight_2.y
						<< "  G3R x=" << glintRight_3.x << " y=" << glintRight_3.y;

				}
			};

			std::vector<calibrationPointCombination> allPoints;

			// temp structure to save all found combinations during calibration on 
			// defined point x as allPointsFoundOnX
			std::vector<pupilGlintCombiInstance> tempFoundPoints;		 // saves all positions of glint and pupil in 1 frame during one cross
			std::vector<pupilGlintCombiInstance> allPointsFoundOn1;		// saves all positions of glint and pupil in 1 frame during one cross
			std::vector<pupilGlintCombiInstance> allPointsFoundOn2;		// => calculate mean from all points of onscreen point 1 and save resulting point in point1, point2...
			std::vector<pupilGlintCombiInstance> allPointsFoundOn3;
			std::vector<pupilGlintCombiInstance> allPointsFoundOn4;
			std::vector<pupilGlintCombiInstance> allPointsFoundOn5;
			std::vector<pupilGlintCombiInstance> allPointsFoundOn6;
			std::vector<pupilGlintCombiInstance> allPointsFoundOn7;
			std::vector<pupilGlintCombiInstance> allPointsFoundOn8;
			std::vector<pupilGlintCombiInstance> allPointsFoundOn9;

			calibrationPointCombination point1, point2, point3, point4, point5, point6, point7, point8, point9;

			vector<Point2d> testPoints;
			vector<Point2d> calcPoints1;
			vector<Point2d> calcPoints2;


	private:

		QGraphicsScene *scene;
		bool m_bIsCaptureRunning;		// FLAG, capturing running or capturing stopped
		double FPS;
		double exposureTime;

	public slots:
		_int64 getCurTime();
		void UpdateGUI();
		void collect();
		Point2f estimate(Point2d original);
		void captureFrame();
		void saveVideoSmall(Mat frame1, Mat frame2);
		vector<Rect> getEyeRegionsByBrightestPoints(Mat frame1, Mat frame2);
		void saveFrames();
		void initializeVideoWriter();
		void showPlayback();
		void savePicture(int number, cv::Mat frame1, cv::Mat frame2);
		void compare();
		Point2d searchInSubArea(Point2d fatherPoint, Mat fatherArea, boolean left);
		Point2d getBrightestPoint(Mat brightFrame);
		pupilGlintCombiInstance detectGlintAndPupil(cv::Mat frame1, cv::Mat frame2);
		RotatedRect fitEllipse_BHO(Mat img);
		void equalizeGlints(Mat * frame, vector<Point2d> glints);
		Point getPupil(Mat src);
		GoalkeeperAnalysis::pupilGlintCombiInstance calcPupilGlintCombi(Point2d pupilRightWholeRef, Point2d pupilLeftWholeRef, vector<Point2d> glintsLeft, vector<Point2d> glintsRight, int leftEyeMarginLeft, int leftEyeMarginTop, int rightEyeMarginLeft, int rightEyeMarginTop);
		void testDraw(pupilGlintCombiInstance cur, Mat * frame);
		cv::Point2d detectPupil_SimpleBlobDetector(cv::Mat src);
		void showGaze();
		void getSplit(vector<Point2d> glints, Rect & eyes);
		void readCameraSettings();
		void calibrate();
		void getLiveView();
		int angle(cv::Point A, cv::Point B);
		void showCalibration();
		void testingGaze();
		void playVideo();
		void analyzeVideo();
		void drawGaze();
		cv::RotatedRect detectPupilELSE(Mat frame1, Mat frame2);
		void testExcuse();
		vector<Rect> getEyeRegion(cv::Mat frame);
		cv::Mat getDifferenceImage(cv::Mat * frame1, cv::Mat * frame2, int threshold, boolean glint);
		std::vector<Point2d> detectGlints(cv::Mat brightFrame, cv::Mat darkFrame, Point2d brightestPoint);
		void stopThreads();
		void writeUserVideoData();
		void startThreads();
		boolean userIsReady();
		void getDesktopResolution(int& horizontal, int& vertical);
		Point2d calculateEyeVector(Point2d pupil, Point2d glint1, Point2d glint2, Point2d glint3);
		double getLengthOfVector(Point2d vecIn);
		void showResults();
		void getMeanValues(std::vector<pupilGlintCombiInstance> allPointsFoundOn, calibrationPointCombination *tmpP);
		void GetDialogOutput();
		
};



class keyEnterReceiver : public QObject
{
	Q_OBJECT


public:

	boolean esc_pressed = false;
protected:
	bool keyEnterReceiver::eventFilter(QObject* obj, QEvent* event)
	{ 
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent* key = static_cast<QKeyEvent*>(event);
			if ((key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return))
			{
				esc_pressed = true;
			}
			else {
				return QObject::eventFilter(obj, event);
			}
			return true;
		}
		else {
			return QObject::eventFilter(obj, event);
		}
		return false;
	}
};