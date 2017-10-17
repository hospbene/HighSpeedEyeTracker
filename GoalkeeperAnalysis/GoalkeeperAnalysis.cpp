#include "GoalkeeperAnalysis.h"

#include "GazeEstimationMethod.h"
#include "PolyFit.h"
#include <string>
#include <qfiledialog.h>
//#include <algo.h>
#include <tiba.h>
#include <Manual.h>
#include <cmath>

#include "stdafx.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>

#include <iostream>            
#include <fstream>
#include <time.h>
#include <sstream>
#include <iomanip>

#include "algo.h"

// Fixed variables
#define SCREEN_WIDTH  1680
#define SCREEN_HEIGHT 1050
#define APERTURE_WIDTH  800
#define APERTURE_HEIGHT 600

#define HARDWARE_GAIN 255
#define GAMMA	220
#define HW_GAIN 100

#define PUPILSIZE 6
#define FIRST_EYE_THRESH 254
#define SECOND_EYE_THRESH 252
#define GLINT_THRESH 216

/** Global variables */
cv::CascadeClassifier eye_cascade;

enum STATE {
	CALIBRATION,
	SAVING,
	TESTING
};

STATE state;
struct sortByX
{
	bool operator() (const Point2d &lhs, const Point2d &rhs)
	{
		return lhs.x < rhs.x;
	}

} mySort;



Point2d oldPupilCenterLeft = 0;
Point2d oldPupilCenterRight = 0;
string userPath;
string userFile;

// Objects
uEyeCamera cam;					// uEye Camera object
CameraCalibration camCalib;		// Camera calibration object
SubjectCalibration subCalib;	// subject calibration object
cv::VideoWriter videoWriter;




// CALIBRATION
//PolyFit gemLeft = PolyFit::PolyFit(PolyFit::POLY_1_X_Y_XY);							//=> only up and down is projected
//PolyFit gemLeft = PolyFit::PolyFit(PolyFit::POLY_1_X_Y_XY_XX_YY);						//both motls extremely small
//PolyFit gemLeft = PolyFit::PolyFit(PolyFit::POLY_1_X_Y_XY_XX_YY_XXYY);				//=> diagonal in the beginning
PolyFit gemLeft = PolyFit::PolyFit(PolyFit::POLY_1_X_Y_XY_XX_YY_XYY_YXX);				//=> best Result for right EyeVector 
//PolyFit gemLeft = PolyFit::PolyFit(PolyFit::POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY);		// best results. 

//PolyFit gemLeft = PolyFit::PolyFit(PolyFit::POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXX_YYY);  //=> -1 -1 

// DEBUG
boolean drawDetection = true;
boolean calibrated = false;
boolean showDetection = false;


// Key press event handler
keyEnterReceiver* keyPressedReceiver = new keyEnterReceiver();


// Adjustable Trackbar values
int firstEyeThresh = FIRST_EYE_THRESH;
int secondEyeThresh = SECOND_EYE_THRESH;
int glintLower = GLINT_THRESH;

int gain = HARDWARE_GAIN;
int gamma = GAMMA;
int hwGain = HW_GAIN;




int contourEllipseSlider = 120; 
int minboxW = 20;
int maxboxW = 97;
int pupilSize = PUPILSIZE;



// Trackbar functions
void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
	}
	else if (event == EVENT_RBUTTONDOWN)
	{
		cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
	}
	else if (event == EVENT_MBUTTONDOWN)
	{
		cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
	}
	else if (event == EVENT_MOUSEMOVE)
	{
		//cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;

	}
}

static void onGammaChange(int v, void *ptr)
{
	// resolve 'this':
	GoalkeeperAnalysis *that = (GoalkeeperAnalysis*)ptr;
	that->changeGamma(v);
}

static void onGainChange(int v, void *ptr)
{
	// resolve 'this':
	GoalkeeperAnalysis *that = (GoalkeeperAnalysis*)ptr;
	that->changeGain(v);
}

static void onHWGainChange(int v, void *ptr)
{
	// resolve 'this':
	GoalkeeperAnalysis *that = (GoalkeeperAnalysis*)ptr;
	that->changeHWGain(v);
}






GoalkeeperAnalysis::GoalkeeperAnalysis(QWidget *parent) : QMainWindow(parent)
{
	// QT STUFF
	ui.setupUi(this);
	scene = new QGraphicsScene(this);

// GUI SLOTS ============================================================================
	connect(ui.showPlaybackButton, SIGNAL(clicked()), this, SLOT(showPlayback()));			// Connect Open camera Button with a function
	connect(ui.startCalibrationButton, SIGNAL(clicked()), this, SLOT(calibrate()));			// Connect Close camera Button with a function
	connect(ui.startCaptureButton, SIGNAL(clicked()), this, SLOT(getLiveView()));			// connect take Snapshot button with function
	connect(ui.stopCaptureButton, SIGNAL(clicked()), this, SLOT(stopThreads()));			// Stop all running threads
	connect(ui.getTestGaze, SIGNAL(clicked()), this, SLOT(testingGaze()));					// show test image and print live captured gaze on it
	
// Windows for showing images
	namedWindow("Frame1", cv::WINDOW_KEEPRATIO | cv::WINDOW_AUTOSIZE);

//set the callback function for any mouse event
	setMouseCallback("Frame1", CallBackFunc, NULL);
	namedWindow("Frame2", cv::WINDOW_KEEPRATIO | cv::WINDOW_AUTOSIZE);
	setMouseCallback("Frame2", CallBackFunc, NULL);
	namedWindow("Difference", cv::WINDOW_KEEPRATIO | cv::WINDOW_AUTOSIZE);
	namedWindow("UserVideo", CV_WINDOW_NORMAL);
	namedWindow("RightEye", cv::WINDOW_KEEPRATIO | cv::WINDOW_AUTOSIZE);
	namedWindow("LeftEye", cv::WINDOW_KEEPRATIO | cv::WINDOW_AUTOSIZE);
	namedWindow("Control0", CV_WINDOW_NORMAL);
	createTrackbar("1st Eye Thresh:", "Control0", &firstEyeThresh, 255);
	createTrackbar("2nd Eye Thresh:", "Control0", &secondEyeThresh, 255);
	createTrackbar("Glint Thresh", "Control0", &glintLower, 255);
	createTrackbar("Gain", "Control0", &gain, 255,onGainChange,this);
	createTrackbar("Gamma", "Control0", &gamma, 220,onGammaChange,this);
	createTrackbar("HW_Gain", "Control0", &hwGain, 220, onHWGainChange, this);
	createTrackbar("PupilSize", "Control0", &pupilSize, 255);


// CALIBRATION STUFF
	//camCalib.start();
	//subCalib.setParameters(camCalib.focalLength, camCalib.principalPoint, camCalib.rotationMatrix, camCalib.translationVector, camCalib.principalPoint_pixels_c, camCalib.principalPoint_pixels_r, camCalib.pixelPitch_mm_c, camCalib.pixelPitch_mm_r);


	eye_cascade.load("haarcascade_eye.xml");		// Load Cascade for EyeRegionDetection
	outputfile.open("data.txt");					// Open Files for data output


	FPS = 500;										// Set Default values
	exposureTime = 1.6;
	cam.setGamma(gamma);
	cam.setGain(gain);
	cam.setGainBoost();
	cam.setHWGain(100);
	readCameraSettings();							// Read settings of uEye camera

	
	//createUserFiles();							// create new user folder and upload video
	
	this->installEventFilter(keyPressedReceiver);	// install thread safe key pressed event handling
	

}


void GoalkeeperAnalysis::createUserFiles()
{
	// get name of user
	dialog = new Dialog;
	dialog->setWindowTitle("Auswahl Stimulusvideo");
	dialog->setWindowFlags(Qt::WindowStaysOnTopHint);
	connect(dialog, SIGNAL(accepted()), this, SLOT(GetDialogOutput()));
	dialog->show();

}


bool copy_file(const char* From, const char* To, std::size_t MaxBufferSize = 1048576)
{
	std::ifstream is(From, std::ios_base::binary);
	std::ofstream os(To, std::ios_base::binary);

	std::pair<char*, std::ptrdiff_t> buffer;
	buffer = std::get_temporary_buffer<char>(MaxBufferSize);

	//Note that exception() == 0 in both file streams,
	//so you will not have a memory leak in case of fail.
	Mat img(Mat(300, 300, CV_8U));
	img = cv::Scalar(50);    // or the desired uint8_t value from 0-255


	// TODO: show Progress Bar
		// imshow("Copy", img);
		// show progress bar
	while (is.good() && os)
	{
		is.read(buffer.first, buffer.second);
		os.write(buffer.first, is.gcount());
	}
	//destroyWindow("Copy");

	std::return_temporary_buffer(buffer.first);

	if (os.fail()) return false;
	if (is.eof()) return true;
	return false;
}


void GoalkeeperAnalysis::GetDialogOutput()
{
	
		bool Opt1, Opt2, Opt3;
		qDebug() << dialog->getOutput();


		// get Name of user and create folder 
		QString str1 = dialog->getOutput();
		string str = str1.toLocal8Bit().constData();
		string path_str = "D:/" + str+"/";

		boost::filesystem::path userFolder(path_str);
		if (boost::filesystem::create_directory(userFolder))
		{
			std::cout << "Success" << "\n";

		}

		// new folder = userPath
		userPath = path_str;
		userFile = userPath+ "stimulus.avi";
			
		// Upload video 
		QString file1Name = QFileDialog::getOpenFileName(this, tr("Save video"), "C:");
		qDebug() << file1Name;

		// user video is under uploadedVideo
		uploadedVideo = file1Name.toStdString();

		qDebug() << "Uservideo: " << QString::fromStdString(uploadedVideo);
		qDebug() << "userFile: " << QString::fromStdString(userFile);

		

		bool CopyResult = copy_file(uploadedVideo.c_str(), userFile.c_str());
		
		if (CopyResult)
		{
			qDebug() << "File successfully copied";
		}
		else
		{
			qDebug() << "File NOT copied";
		}
}

// ========================================================================== INITIAL SETUP
void GoalkeeperAnalysis::readCameraSettings()
{

	cam.getSensorInformation();
	cam.activateInternalMemory();
	cam.getCameraInformation();
	cam.InitMemory();
	cam.setPixelclock();
	cam.setFrameRate(FPS);
	cam.setExposure(exposureTime);
	cam.setFlash();

}

// ========================================================================== SETTINGS FOR EACH BUTTON FUNCTION
// CALIBRATION SETTINGS: initialize settings for capture and detection with low speed (for calibration)
void GoalkeeperAnalysis::calibrate()
{
	// WOLFGANG
		//initializeVideoWriter();
	cam.setFrameRate(25);	// 25		Arduino 2500
	cam.setExposure(4.0);	//4.0
	cam.InitMemory();
	cam.setFlash();

	state = CALIBRATION;
	startThreads();
}

// TESTING SETTINGS:
void GoalkeeperAnalysis::testingGaze()
{

	cam.setFrameRate(25);	// 25		Arduino 2500
	cam.setExposure(4.0);	//4.0
	cam.InitMemory();
	cam.setFlash();

	state = TESTING;
	startThreads();

}

// SAVING SETTINGS: initialize settings for capturing in high speed
void GoalkeeperAnalysis::getLiveView()
{
	initializeVideoWriter();
	cam.InitMemory();
	cam.setFrameRate(FPS);
	cam.setExposure(1.6);
	cam.setFlash();
	cam.brightenUpCapturing();
	state = SAVING;
	startThreads();
}


// ========================================================================== START STOP FUNCTIONS FOR EACH THREADCOMBI
void GoalkeeperAnalysis::startThreads()
{
	// 1. If cam not opened, open it
	if (!cam.camOpened)
	{
		cam.openCamera();
	}

	// 2. Signalising uEye to start capturing
	if (cam.startCapture())
	{
		qDebug() << "is_CaptureVideo: success";
	}

	// 3. Set flag for capturing thread
	m_bIsCaptureRunning = true;

	// 4. According to set state start threads
	// Thread 1 is capturing Thread and thread 2 depends on the state
	v.emplace_back(&GoalkeeperAnalysis::captureFrame, this);

	switch (state)
	{
	case SAVING:

		// wait for user to press spacebar
		while (!userIsReady());

		v.emplace_back(&GoalkeeperAnalysis::saveFrames, this);
		v.emplace_back(&GoalkeeperAnalysis::playVideo, this);
		break;
	case CALIBRATION:
		v.emplace_back(&GoalkeeperAnalysis::showCalibration, this);

		// WOLFGANG
				//v.emplace_back(&GoalkeeperAnalysis::saveFrames, this);

		break;
	case TESTING:
		v.emplace_back(&GoalkeeperAnalysis::showGaze, this);
		break;
	default:
		break;

	}

	UpdateGUI();

	qDebug() << "Video capture started";
}

boolean GoalkeeperAnalysis::userIsReady()
{


	namedWindow("UserVideo", -1);
	cvSetWindowProperty("UserVideo", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
	Mat bg(SCREEN_HEIGHT, SCREEN_WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));
	cv::putText(bg, "To start experiment.", Point2f(40, (SCREEN_HEIGHT /2)-20), FONT_HERSHEY_SIMPLEX, 3.0, Scalar(0, 0, 0, 0), 6);
	cv::putText(bg, "Press SPACE .", Point2f(40, (SCREEN_HEIGHT / 2)+ 120), FONT_HERSHEY_SIMPLEX, 5, Scalar(0, 0, 0, 0), 4);
	imshow("UserVideo",bg);
	
	// Loop until escape is pressed
	// & 0xFF is needed to get the offset opencv is adding
	// 27 is DEC of ESC in ASCII Code
	// 13 is DEC of CARRIAGE RETURN in ASCII CODE
	// 32 is Space bar
	while ((cv::waitKey() & 0xFF) != 32) //27 is the keycode for ESC
	{
		qDebug() << "NOT PRESSED......................................................................";
	}

	return true;
}


void GoalkeeperAnalysis::stopThreads()
{

	// 1. Signalising end of capturing
	m_bIsCaptureRunning = false;
	


	// 2. Tell uEye camera to stop capturing
	while (!cam.stopVideo())
	{
		qDebug() << "Could not stop live capture";
	}

	// 3. Kill all runnning Threads
	for (auto& t : v)
	{
		t.join();
	}

	// 4. Clear memomry of uEyeCamera
	cam.clearMem();

	// 5. update GUI
	UpdateGUI();

	// 6. close outputfile for glints and pupil centers
	if (outputfile.is_open())
	{
		outputfile.close();
	}

	if (videoWriter.isOpened())
	{
		videoWriter.release();
	}

	if (state == SAVING)
	{

		// write userVideo data
		writeUserVideoData();

		// write eyeVideo data and detect pupils and glints and estimate gaze 
		analyzeVideo();

		// draw gaze on userVideo
		drawGaze();
	}

}



void GoalkeeperAnalysis::writeUserVideoData()
{

	// Write UservideoFile 
	userVideoFile.open(userPath + "stimulus.txt");								// open file to write frame2time combination
	qDebug() << "User Video Frames2Timestamp:";
	qDebug() << "Start: " << userVideoDictionary.startTime;
	userVideoFile << "Start: " << userVideoDictionary.startTime << endl;		// PRINT: start time of userVideo

	for (int i = 0; i < userVideoDictionary.frame2time.size(); i++)
	{
		qDebug() << "Frame: " << i << " is " << userVideoDictionary.frame2time[i];
		userVideoFile << "Frame " << i << ": " << userVideoDictionary.frame2time[i];

	}

	userVideoFile << "End: " << userVideoDictionary.endTime;
	qDebug() << "End: " << userVideoDictionary.endTime;

	// close file
	if (userVideoFile.is_open())
	{
		userVideoFile.close();
	}
}

// ========================================================================== THREAD FUNCTIONS

// SAVING THREAD 2: saveFrames()
void GoalkeeperAnalysis::saveFrames()
{


	eyeVideoDictionary.startTime = getCurTime();

	int curFrameNumber = 0;

	while (m_bIsCaptureRunning)
	{
		if (!framesQueue.empty())
		{
			try
			{
				sharedMutex.lock();
					videoWriter.write(framesQueue.front());
						eyeVideoDictionary.frame2time.emplace(curFrameNumber, getCurTime()-eyeVideoDictionary.startTime);
					curFrameNumber++;
					framesQueue.pop();
				sharedMutex.unlock();
			}
			catch (cv::Exception& ex)
			{ //OpenCV Exception
				cerr << "OpenCV Error:\n" << ex.what();
				qDebug() << ex.code;
			}
			catch (runtime_error& ex)
			{
				cerr << "Runtime Error:\n" << ex.what();
			}
			catch (...)
			{
				cerr << "Generic Error";
			}
		}
	}

	eyeVideoDictionary.endTime = getCurTime();



}

// CALIBRATION THREAD 2:
void GoalkeeperAnalysis::showCalibration()
{


	
	namedWindow("Calibration", CV_WINDOW_AUTOSIZE | CV_WINDOW_KEEPRATIO);
	setMouseCallback("Calibration", CallBackFunc, NULL);

	
	//getDesktopResolution(SCREEN_WIDTH, SCREEN_HEIGHT);
	byte timeToShowEachCross = 5;


	Point2d firstCross = cv::Point(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 2);
	Point2d secondCross = cv::Point(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 2);
	Point2d thirdCross = cv::Point(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 2);
	Point2d fourthCross = cv::Point(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 7);
	Point2d fifthCross = cv::Point(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 7);
	Point2d sixthCross = cv::Point(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 7);
	Point2d seventhCross = cv::Point(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 12);
	Point2d eighthCross = cv::Point(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 12);
	Point2d ninethCross = cv::Point(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 12);

	qDebug() << firstCross.x << firstCross.y << endl;
	qDebug() << secondCross.x << secondCross.y << endl;
	qDebug() << thirdCross.x << thirdCross.y << endl;
	qDebug() << fourthCross.x << fourthCross.y << endl;
	qDebug() << fifthCross.x << fifthCross.y << endl;
	qDebug() << sixthCross.x << sixthCross.y << endl;
	qDebug() << seventhCross.x << seventhCross.y << endl;
	qDebug() << eighthCross.x << eighthCross.y << endl;
	qDebug() << ninethCross.x << ninethCross.y << endl;

	// Initial Instruction window
	Mat bg(SCREEN_HEIGHT, SCREEN_WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));
	cv::putText(bg, "To calibrate the system you need to focus on the 9 different spots shown below sequentially. Please focus on the center of the cross ", Point2f(20, 20), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0, 0), 1);
	cv::putText(bg, "as long as it is shown. Press any key to start calibration and to continue. Please focus each number for about 2 seconds.", Point2f(20, 40), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0, 0), 1);


	// 1 Cross
	cv::drawMarker(bg, firstCross, cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);              // 1     0,0
	cv::circle(bg, firstCross, 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "1", Point2d(firstCross.x + 20, firstCross.y - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point1.centerOfCrossOnScreen = firstCross;
	point1.name = "Point 1";


	// 2 Cross
	cv::drawMarker(bg, secondCross, cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 2     0,1
	cv::circle(bg, secondCross, 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "2", Point2d(secondCross.x + 20, secondCross.y - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point2.centerOfCrossOnScreen = secondCross;
	point2.name = "Point 2";

	// 3 Cross
	cv::drawMarker(bg, thirdCross, cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 3     0,2
	cv::circle(bg, thirdCross, 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "3", Point2d(thirdCross.x + 20, thirdCross.y - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point3.centerOfCrossOnScreen = thirdCross;
	point3.name = "Point 3";


	// 4 Cross
	cv::drawMarker(bg, fourthCross, cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);              // 4     1,0
	cv::circle(bg, fourthCross, 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "4", Point2d(fourthCross.x + 20, fourthCross.y - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point4.centerOfCrossOnScreen = fourthCross;
	point4.name = "Point 4";

	// 5 Cross
	cv::drawMarker(bg, fifthCross, cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 5     1,1 
	cv::circle(bg, fifthCross, 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "5", Point2d(fifthCross.x + 20, fifthCross.y - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point5.centerOfCrossOnScreen = fifthCross;
	point5.name = "Point 5";


	// 6 Cross
	cv::drawMarker(bg, sixthCross, cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 6     1,2
	cv::circle(bg, sixthCross, 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "6", Point2d(sixthCross.x + 20, sixthCross.y - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point6.centerOfCrossOnScreen = sixthCross;
	point6.name = "Point 6";


	// 7 Cross
	cv::drawMarker(bg, seventhCross, cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);             // 7     2,0
	cv::circle(bg, seventhCross, 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "7", Point2d(seventhCross.x + 20, seventhCross.y - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point7.centerOfCrossOnScreen = seventhCross;
	point7.name = "Point 7";


	// 8 Cross
	cv::drawMarker(bg, eighthCross, cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);           // 8     2,1
	cv::circle(bg, eighthCross, 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "8", Point2d(eighthCross.x + 20, eighthCross.y - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point8.centerOfCrossOnScreen = eighthCross;
	point8.name = "Point 8";


	// 9 Cross
	cv::drawMarker(bg, ninethCross, cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);           // 9     2,2
	cv::circle(bg, ninethCross, 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "9", Point2d(ninethCross.x + 20, ninethCross.y - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point9.centerOfCrossOnScreen = ninethCross;
	point9.name = "Point 9";

	/*
	qDebug() << point1.centerOfCrossOnScreen.x << point1.centerOfCrossOnScreen.y << endl;
	qDebug() << point2.centerOfCrossOnScreen.x << point2.centerOfCrossOnScreen.y << endl;
	qDebug() << point3.centerOfCrossOnScreen.x << point3.centerOfCrossOnScreen.y << endl;
	qDebug() << point4.centerOfCrossOnScreen.x << point4.centerOfCrossOnScreen.y << endl;
	qDebug() << point5.centerOfCrossOnScreen.x << point5.centerOfCrossOnScreen.y << endl;
	qDebug() << point6.centerOfCrossOnScreen.x << point6.centerOfCrossOnScreen.y << endl;
	qDebug() << point7.centerOfCrossOnScreen.x << point7.centerOfCrossOnScreen.y << endl;
	qDebug() << point8.centerOfCrossOnScreen.x << point8.centerOfCrossOnScreen.y << endl;
	qDebug() << point9.centerOfCrossOnScreen.x << point9.centerOfCrossOnScreen.y << endl;
	*/

	cv::imshow("Calibration", bg);
	cv::waitKey();
	tempFoundPoints.clear();
	bg = Scalar(255, 255, 255, 0);

	// Start sequence
	outputfile << "Cross1";


	cv::drawMarker(bg, cv::Point(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 2), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);              // 1     0,0
	cv::circle(bg, cv::Point(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 2), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "1", Point2f((SCREEN_WIDTH / 10) + 20, (SCREEN_HEIGHT / 15 * 2) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	cv::imshow("Calibration", bg);
	waitKey(1);
	boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
	allPointsFoundOn1 = tempFoundPoints;		// save all found positions saved in temp structure in structure 1
	tempFoundPoints.clear();

	outputfile << "Cross2" << endl;
	bg = Scalar(255, 255, 255, 0);
	cv::drawMarker(bg, cv::Point(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 2), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 2     0,1
	cv::circle(bg, cv::Point(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 2), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "2", Point2f((SCREEN_WIDTH / 10 * 5) + 20, (SCREEN_HEIGHT / 15 * 2) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	cv::imshow("Calibration", bg);
	waitKey(1);
	boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
	allPointsFoundOn2 = tempFoundPoints;		// save all found positions saved in temp structure in structure 2
	tempFoundPoints.clear();
	outputfile << "Cross3" << endl;

	bg = Scalar(255, 255, 255, 0);
	cv::drawMarker(bg, cv::Point(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 2), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 3     0,2
	cv::circle(bg, cv::Point(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 2), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "3", Point2f((SCREEN_WIDTH / 10 * 9) + 20, (SCREEN_HEIGHT / 15 * 2) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	cv::imshow("Calibration", bg);
	waitKey(1);
	boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
	allPointsFoundOn3 = tempFoundPoints;		// save all found positions saved in temp structure in structure 3
	tempFoundPoints.clear();
	outputfile << "Cross4" << endl;

	bg = Scalar(255, 255, 255, 0);
	cv::drawMarker(bg, cv::Point(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 7), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);              // 4     1,0
	cv::circle(bg, cv::Point(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 7), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "4", Point2f((SCREEN_WIDTH / 10) + 20, (SCREEN_HEIGHT / 15 * 7) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	cv::imshow("Calibration", bg);
	waitKey(1);
	boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
	allPointsFoundOn4 = tempFoundPoints;		// save all found positions saved in temp structure in structure 4
	tempFoundPoints.clear();
	outputfile << "Cross5" << endl;

	bg = Scalar(255, 255, 255, 0);
	cv::drawMarker(bg, cv::Point(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 7), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 5     1,1 
	cv::circle(bg, cv::Point(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 7), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "5", Point2f((SCREEN_WIDTH / 10 * 5) + 20, (SCREEN_HEIGHT / 15 * 7) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	cv::imshow("Calibration", bg);
	waitKey(1);
	boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
	allPointsFoundOn5 = tempFoundPoints;		// save all found positions saved in temp structure in structure 5
	tempFoundPoints.clear();
	outputfile << "Cross6" << endl;

	bg = Scalar(255, 255, 255, 0);
	cv::drawMarker(bg, cv::Point(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 7), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 6     1,2
	cv::circle(bg, cv::Point(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 7), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "6", Point2f((SCREEN_WIDTH / 10 * 9) + 20, (SCREEN_HEIGHT / 15 * 7) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	cv::imshow("Calibration", bg);
	waitKey(1);
	boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
	allPointsFoundOn6 = tempFoundPoints;		// save all found positions saved in temp structure in structure 6
	tempFoundPoints.clear();
	outputfile << "Cross7" << endl;

	bg = Scalar(255, 255, 255, 0);
	cv::drawMarker(bg, cv::Point(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 12), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);             // 7     2,0
	cv::circle(bg, cv::Point(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 12), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "7", Point2f((SCREEN_WIDTH / 10) + 20, (SCREEN_HEIGHT / 15 * 12) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	cv::imshow("Calibration", bg);
	waitKey(1);
	boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
	allPointsFoundOn7 = tempFoundPoints;		// save all found positions saved in temp structure in structure 7
	tempFoundPoints.clear();
	outputfile << "Cross8" << endl;

	bg = Scalar(255, 255, 255, 0);
	cv::drawMarker(bg, cv::Point(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 12), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);           // 8     2,1
	cv::circle(bg, cv::Point(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 12), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "8", Point2f((SCREEN_WIDTH / 10 * 5) + 20, (SCREEN_HEIGHT / 15 * 12) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	cv::imshow("Calibration", bg);
	waitKey(1);
	boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
	allPointsFoundOn8 = tempFoundPoints;		// save all found positions saved in temp structure in structure 8
	tempFoundPoints.clear();
	outputfile << "Cross9" << endl;

	bg = Scalar(255, 255, 255, 0);
	cv::drawMarker(bg, cv::Point(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 12), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);           // 9     2,2
	cv::circle(bg, cv::Point(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 12), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	cv::putText(bg, "9", Point2f((SCREEN_WIDTH / 10 * 9) + 20, (SCREEN_HEIGHT / 15 * 12) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	cv::imshow("Calibration", bg);
	waitKey(1);
	boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
	allPointsFoundOn9 = tempFoundPoints;		// save all found positions saved in temp structure in structure 9
	tempFoundPoints.clear();

	bg = Scalar(255, 255, 255, 0);
	cv::putText(bg, "Calibration end. Showing results in 3 seconds. ", Point2f(20, 20), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0, 0), 1);
	waitKey(1);
	boost::this_thread::sleep(boost::posix_time::milliseconds(3000));

	if (tempFoundPoints.empty())
	{
		qDebug() << "temp is empty";
	}
	else
	{
		// Print every found point. glint left and right and pupil left and right. for every frame and on every calibration point
		if (true)
		{
			qDebug() << "\n\n\n Cross 1";
			for (std::vector<pupilGlintCombiInstance>::iterator it = allPointsFoundOn1.begin(); it != allPointsFoundOn1.end(); it++)
			{
				it->toString();
			}

			qDebug() << "\n Cross 2";
			for (std::vector<pupilGlintCombiInstance>::iterator it = allPointsFoundOn2.begin(); it != allPointsFoundOn2.end(); it++)
			{
				it->toString();
			}
			qDebug() << "\n Cross 3";
			for (std::vector<pupilGlintCombiInstance>::iterator it = allPointsFoundOn3.begin(); it != allPointsFoundOn3.end(); it++)
			{
				it->toString();
			}
			qDebug() << "\n Cross 4";
			for (std::vector<pupilGlintCombiInstance>::iterator it = allPointsFoundOn4.begin(); it != allPointsFoundOn4.end(); it++)
			{
				it->toString();
			}
			qDebug() << "\n Cross 5";
			for (std::vector<pupilGlintCombiInstance>::iterator it = allPointsFoundOn5.begin(); it != allPointsFoundOn5.end(); it++)
			{
				it->toString();
			}
			qDebug() << "\n Cross 6";
			for (std::vector<pupilGlintCombiInstance>::iterator it = allPointsFoundOn6.begin(); it != allPointsFoundOn6.end(); it++)
			{
				it->toString();
			}
			qDebug() << "\n Cross 7";
			for (std::vector<pupilGlintCombiInstance>::iterator it = allPointsFoundOn7.begin(); it != allPointsFoundOn7.end(); it++)
			{
				it->toString();
			}
			qDebug() << " \nCross 8";
			for (std::vector<pupilGlintCombiInstance>::iterator it = allPointsFoundOn8.begin(); it != allPointsFoundOn8.end(); it++)
			{
				it->toString();
			}
			qDebug() << "\n Cross 9";
			for (std::vector<pupilGlintCombiInstance>::iterator it = allPointsFoundOn9.begin(); it != allPointsFoundOn9.end(); it++)
			{
				it->toString();
			}
		}


		// 1. Get mean value for glints and pupil in each cross
		// allPointsFoundOn1 to  allPointsFoundOn9 contains all Found glint positions and pupil positiones found at cross X
		// mean Values are stored in structure point1 to point9

		// Calc Mean values of glints and pupil centers for each calibration point
		getMeanValues(allPointsFoundOn1, &point1);
		getMeanValues(allPointsFoundOn2, &point2);
		getMeanValues(allPointsFoundOn3, &point3);
		getMeanValues(allPointsFoundOn4, &point4);
		getMeanValues(allPointsFoundOn5, &point5);
		getMeanValues(allPointsFoundOn6, &point6);
		getMeanValues(allPointsFoundOn7, &point7);
		getMeanValues(allPointsFoundOn8, &point8);
		getMeanValues(allPointsFoundOn9, &point9);

		// 2. Save as global structure all mean points and the corresponding screenpoint
		allPoints.push_back(point1);
		allPoints.push_back(point2);
		allPoints.push_back(point3);
		allPoints.push_back(point4);
		allPoints.push_back(point5);
		allPoints.push_back(point6);
		allPoints.push_back(point7);
		allPoints.push_back(point8);
		allPoints.push_back(point9);

		// 3. Create Matrix for Eye Left and Right by creating eyevectors for each combination
		m_bIsCaptureRunning = false;

			// Feed Thiagos algorithm
			collect();
		
	}
	
}

// input is 
void GoalkeeperAnalysis::collect()
{
		
// PupilVector
	vector<CollectionTuple> tuples;
	for (int i = 0; i < allPoints.size(); i++)
	{
	

		// Pupil only
		//Point2d eyeVec = allPoints[i].pupilLeft;
		//Point2d eyeVec = allPoints[i].pupilRight;


		// GLints Only 
		//Point2d eyeVec = allPoints[i].glintLeft_1 + allPoints[i].glintLeft_2 + allPoints[i].glintLeft_3;
		//Point2d eyeVec = allPoints[i].glintRight_1 + allPoints[i].glintRight_2 + allPoints[i].glintRight_3;				// BEST RESULT SO FAR


		// Pupil and Glint
		Point2d eyeVec = calculateEyeVector(allPoints[i].pupilLeft, allPoints[i].glintLeft_1, allPoints[i].glintLeft_2, allPoints[i].glintLeft_3);
		//Point2d eyeVec = calculateEyeVector(allPoints[i].pupilRight, allPoints[i].glintRight_1, allPoints[i].glintRight_2, allPoints[i].glintRight_3);
		

		EyeData ed;
		ed.pupil.center.x = eyeVec.x;
		ed.pupil.center.y = eyeVec.y;


		FieldData fd;
		fd.collectionMarker.center.x = allPoints[i].centerOfCrossOnScreen.x;
		fd.collectionMarker.center.y = allPoints[i].centerOfCrossOnScreen.y;
		qDebug() << "Screen: " << allPoints[i].centerOfCrossOnScreen.x << allPoints[i].centerOfCrossOnScreen.y;

		// Add to structure
		DataTuple dt(0, ed, EyeData(), fd);
		tuples.push_back(CollectionTuple(dt));
	}

	QString error;
	calibrated = gemLeft.calibrate(tuples, GazeEstimationMethod::MONO_LEFT, error);
	if (!calibrated)
		qDebug() << error;
	
	
}


// TODO: add glints
Point2f GoalkeeperAnalysis::estimate(Point2d original)
{
	Point2f corrected = Point(200, 200);
	//if (calibrated) 
	//{
		EyeData ed;
		ed.pupil.center = original;
		DataTuple dt(0, ed, EyeData(), FieldData());
		 corrected = to2D(gemLeft.estimateGaze(dt, GazeEstimationMethod::MONO_LEFT));
		 qDebug() << "Gaze: " << corrected;
		
//	}
	return corrected;

}

// THREAD ( test gaze 2): reads queue and shows each found point on screen
void GoalkeeperAnalysis::showGaze()
{
	// read queue and show points on screen

	Mat bg(SCREEN_HEIGHT, SCREEN_WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));

	while (m_bIsCaptureRunning)
	{
		sharedMutex.lock();
		if (!gazeQueue.empty())
		{

		
			bg.setTo(cv::Scalar::all(0));
			qDebug() << "Queue front.";
			pupilGlintCombiInstance d = gazeQueue.front();
			gazeQueue.pop();
			
			/*
			Point2d d2 = d.pupilLeft;
			cv::drawMarker(bg, estimate(d2), cv::Scalar(0, 255, 0), MARKER_CROSS, 10, 2);              // 1     0,0	
			qDebug() << "Pupil: " << d2.x << d2.y;
			cv::circle(bg, d2, 5, Scalar(0, 0, 250));
			cv::circle(bg, estimate(d2), 5, Scalar(0, 0, 0));

			cv::imshow("Gaze", bg);
			waitKey(1);
	
	*/




			Point2d eyeVec = calculateEyeVector(d.pupilLeft, d.glintLeft_1, d.glintLeft_2, d.glintLeft_3);
			//Point2d eyeVec = calculateEyeVector(d.pupilRight, d.glintRight_1, d.glintRight_2, d.glintRight_3);
			
//			Point2d eyeVec = d.glintRight_1 + d.glintRight_2 + d.glintRight_3;

			qDebug() << "EyeVec: " << eyeVec.x << eyeVec.y;
			cv::circle(bg, eyeVec, 5, Scalar(0, 0, 250));
			cv::circle(bg, estimate(eyeVec), 5, Scalar(0,0,0));

			cv::drawMarker(bg, estimate(eyeVec), cv::Scalar(0, 255, 0), MARKER_CROSS, 10, 2);              // 1     0,0	
			cv::imshow("Gaze", bg);
			waitKey(1);
			
		}
		else
		{
			//qDebug() << "Queue is empty.";
		}
		sharedMutex.unlock();
	}




}


// captureFrame()
// capture 2 Frames sequentially and save them in Queue
void GoalkeeperAnalysis::captureFrame()
{
	qDebug() << "Thread for capturing Frames started.";
	threadEndedNormally= false;
	cv::Mat frame1;
	cv::Mat frame2;

	char* pBuffer;
	INT nMemID = 0;
	int camRet;
	double dblFPS;
	int counter = 0;
	int frameNumber = 0;

	outputfile << "Framenumber:" << frameNumber << ";";
	//cam.setGain(gain);
	//cam.setGamma(gamma);

	while (m_bIsCaptureRunning)
	{
		
		if (cam.getNextImage(&pBuffer, &nMemID))
		{
			cam.getCurrentFPS(&dblFPS);
			//qDebug() << "FPS " << dblFPS;

			frame1 = cv::Mat(cam.sensorInfo.nMaxHeight, cam.sensorInfo.nMaxWidth, CV_8UC3, pBuffer);
			cam.unlockSequenceBuffer(&nMemID, &pBuffer);

			if (cam.getNextImage(&pBuffer, &nMemID))
			{

				frame2 = cv::Mat(cam.sensorInfo.nMaxHeight, cam.sensorInfo.nMaxWidth, CV_8UC3, pBuffer);
				cam.unlockSequenceBuffer(&nMemID, &pBuffer);

				if (frame1.data != NULL)
				{

					switch (state)
					{
						case TESTING:
						{

							// 1. detect pupil and glint and return it
							// 2. add gaze points to queue gazesQueue

							// add points to queue
							sharedMutex.lock();
								gazeQueue.push(detectGlintAndPupil(frame1, frame2));
							sharedMutex.unlock();

							//testExcuse();
							
							break;
						}
						case CALIBRATION:
						{

							//savePicture(counter,frame1, frame2);
							//counter+=2;
							
							// 1. detect pupil and glint and return it
							// 2. Save them in structure for calibration points
								pupilGlintCombiInstance pgci = detectGlintAndPupil(frame1, frame2);
							// add current found combination to current vector
								if(pgci.valid)
								{
									tempFoundPoints.push_back(pgci);
								}
								
							// testing
								testDraw(pgci,&frame1);

							//WOLFGANG
								//saveVideoSmall(frame1, frame2);
							cv::imshow("Frame1", frame1);
							cv::imshow("Frame2", frame2);
							break;
						}
						case SAVING:
						{
							// TODO: Show Traffic sign if face is not in range!
							// Maybe show direct where to go


							sharedMutex.lock();
								framesQueue.push(frame1);
								framesQueue.push(frame2);
							sharedMutex.unlock();
							cv::imshow("Frame1",frame1);
							//cv::imshow("Frame2", frame2);
								break;
						}
						default:
							break;
					}
					
			
				}
				frameNumber = frameNumber + 2;
			
			}
		}
		else
		{
			qDebug() << "Camera could NOT get next buffered image.";
		}
	}
		
	frameNumber = 0;
	threadEndedNormally = true;
	if (videoWriter.isOpened())
	{
		videoWriter.release();
		qDebug() << "VideoWriter released";
	}



	if(outputfile.is_open())
	{
		outputfile << endl;
		qDebug() << "Outputfile closed";

	}
	


	qDebug() << "Thread for capturing Frames Ended.";
	

}


void GoalkeeperAnalysis::saveVideoSmall(Mat frame1, Mat frame2)
{

	// save margins from cutted out regions in these variables
	int leftEyeMarginLeft, leftEyeMarginTop, rightEyeMarginLeft, rightEyeMarginTop;
	Mat brightRightEye, brightLeftEye;
	Mat darkRightEye, darkLeftEye;
	Point2d pupilCenterLeft = Point(0, 0);
	Point2d pupilCenterRight = Point(0, 0);
	pupilGlintCombiInstance curPositions;
	Point2d brightestPointRight, brightestPointLeft;

	// all glints
	vector<Point2d> glintsLeft, glintsRight;


	//0. detect bright/dark frame
	//////////////////////////////////////////////////////////////////////////////////////////////

	Scalar frame1Mean = mean(frame1);
	Scalar  frame2Mean = mean(frame2);
	Mat bright;
	Mat dark;

	if (frame1Mean.val[0] > frame2Mean.val[0])
	{
		bright = frame1;
		dark = frame2;
	}
	else
	{
		bright = frame2;
		dark = frame1;
	}

	// 1.a Detect Eyes by brightest Point
	//////////////////////////////////////////////////////////////////////////////////////////////
	if (!bright.empty() && !dark.empty())
	{
		
		
		vector<Rect> eyeRegions = getEyeRegionsByBrightestPoints(frame1, frame2);
		
		eyeRegions.reserve(2);
		// 1. Get separate Regions of Frame for both eyes
		if (!eyeRegions.empty())
		{

			if (eyeRegions.size() > 0)
			{
				if ((0 <= eyeRegions[0].x && 0 <= eyeRegions[0].width && eyeRegions[0].x + eyeRegions[0].width <= bright.cols && 0 <= eyeRegions[0].y && 0 <= eyeRegions[0].height && eyeRegions[0].y + eyeRegions[0].height <= bright.rows))
				{


					brightRightEye = bright(eyeRegions[0]);
					darkRightEye = dark(eyeRegions[0]);

					//cv::resize(brightRightEye, brightRightEye, cv::Size(100, 100));
					//cv::resize(darkRightEye, darkRightEye, cv::Size(100, 100));



				

					// 2. Save margin values of cutted out regions for later back calculating the position in the whole image
					// original postition of point in region is at x: leftEyeMarginTop + distance x in region itself
					//											   y: leftEyeMarginleft + distance y in region itself
					//											   x: rightEyeMarginTop + distance x in region itself
					//											   y: rightEyeMarginleft + distance y in region itself

					rightEyeMarginLeft = eyeRegions[0].x;
					rightEyeMarginTop = eyeRegions[0].y;
				}
			}

			if (eyeRegions.size() > 1)
			{

				if ((0 <= eyeRegions[1].x && 0 <= eyeRegions[1].width && eyeRegions[1].x + eyeRegions[1].width <= bright.cols && 0 <= eyeRegions[1].y && 0 <= eyeRegions[1].height && eyeRegions[1].y + eyeRegions[1].height <= bright.rows))
				{

					brightLeftEye = bright(eyeRegions[1]);
					darkLeftEye = dark(eyeRegions[1]);



					sharedMutex.lock();
						framesQueue.push(brightRightEye);
						framesQueue.push(darkRightEye);
					sharedMutex.unlock();

					qDebug() << "Mat size: rows=" << brightLeftEye.rows << "Cols=" << brightLeftEye.cols;
					// 2. Save margin values of cutted out regions for later back calculating the position in the whole image
					// original postition of point in region is at x: leftEyeMarginTop + distance x in region itself
					//											   y: leftEyeMarginleft + distance y in region itself
					//											   x: rightEyeMarginTop + distance x in region itself
					//											   y: rightEyeMarginleft + distance y in region itself

					leftEyeMarginLeft = eyeRegions[1].x;
					leftEyeMarginTop = eyeRegions[1].y;
				}
			}
		}
		
	}

	/*
	if (!brightLeftEye.empty())
	{
		imshow("LeftEye", brightLeftEye);

	}
	*/

	if (!brightRightEye.empty())
	{
		imshow("RightEye", brightRightEye);
	}
	



}


/*
FUNC: getEyeRegionsByBrightestPoints(Mat bright, Mat dark)
	1. Searches for the brightest point in the image (usually glint) by using differenceImage of bright and dark frame
	2. Blackens area around brightest point and searches (depending on "2nd Eye Thresh:"-Trackbar) for second brightest Point
	3. Regions around these two points are cut from the bright Frame and are given back as eye regions.
	4. Depending of the x Value of the area building brightestPoint the regions are given back and the brightestPoints
		are set globally for the detectGlints function. Because glints should all align with the brightest point (in the same eye).

RETURN: vector of Rectangles. Element 0 left Region in Image and Element 1 is right Region in image
	(if head movement is reduced usually 0 = right eye and 1 = left eye) 

*/
vector<Rect> GoalkeeperAnalysis::getEyeRegionsByBrightestPoints(Mat bright, Mat dark)
{

	boolean draw = false;

	vector<Rect> eyeRegions;
	Rect eyeRoi1, eyeRoi2;
	Point brightestPoint, brightestPoint2;


// 1.Detect Brightest Point in differenceImage.

	// get differenceImage
	cv::Mat differenceImage,differenceImage2;

	differenceImage = getDifferenceImage(&bright, &dark, firstEyeThresh ,false);



	// get brightest Spot in whole image => usually a glint
	double min, max;
	cv::Point min_loc;
	cv::minMaxLoc(differenceImage, &min, &max, &min_loc, &brightestPoint);

	// 3. draw circle of found point
	if (draw)
	{
		circle(bright, brightestPoint, 2, Scalar(0, 0, 255), 1, 8, 0); 
		cv::drawMarker(bright, brightestPoint, cv::Scalar(0, 0, 255), MARKER_CROSS, 5, 1);              // 1     0,0	
	}

	imshow("1st Eye", differenceImage);
	cv::waitKey(1);


	// save rectangle for first eye
	eyeRoi1 = Rect(brightestPoint.x-50, brightestPoint.y - 50, 100, 100);
	if (eyeRoi1.x > 0 && eyeRoi1.y > 0 && eyeRoi1.width > 0 && eyeRoi1.height > 0)
	{
		eyeRegions.push_back(eyeRoi1);
	}

	// draw first reagion in original image
	//cv::rectangle(bright, eyeRoi1, Scalar(0, 0, 0), 1, 8, 0);
	int newY;


// 2. Create new AOI around brightest Point. 200 px high, full image length wide and search for second brightest point
// by setting the first found eye region to total black, a second call of minMaxLoc should return the next brightest point 
// which should be the other eye region
	if (brightestPoint.y > 0)
	{	
		newY = brightestPoint.y - 100;
		Rect newSearchArea = Rect(0, newY, 700, 200);

		if((0 <= newSearchArea.x && 0 <= newSearchArea.width && newSearchArea.x + newSearchArea.width <= differenceImage.cols && 0 <= newSearchArea.y && 0 <= newSearchArea.height && newSearchArea.y + newSearchArea.height <= differenceImage.rows))
		{

			differenceImage2 = getDifferenceImage(&bright, &dark, secondEyeThresh, false);

			Rect blackenedArea;
			blackenedArea.x = eyeRoi1.x;
			blackenedArea.y = eyeRoi1.y;
			blackenedArea.width = eyeRoi1.width + 30;
			blackenedArea.height = eyeRoi1.height + 20;

			// set area around brightestPoint to black to delete brightest Point area (first eye with all its glints)
			cv::rectangle(differenceImage2, blackenedArea, Scalar(0, 0, 0), CV_FILLED, 8, 0);

			Mat tmpMat = differenceImage2(newSearchArea);
			//rectangle(bright, newSearchArea, Scalar(0, 0, 0), 1, 8, 0);






// 3 .Detect Brightest Point in differenceImage again to find second eye region.

			cv::minMaxLoc(tmpMat, &min, &max, &min_loc, &brightestPoint2);

			// calculate Position of second brightest point back from ROI to fathermat
			brightestPoint2.x += newSearchArea.x;
			brightestPoint2.y += newSearchArea.y;

			if (draw)
			{
				// draw found point
				circle(bright, brightestPoint2, 2, Scalar(0, 255, 0), 1, 8, 0);
				cv::drawMarker(bright, brightestPoint2, cv::Scalar(0, 255, 0), MARKER_CROSS, 5, 1);              // 1     0,0

				
			}
			imshow("2nd Eye", differenceImage2);
			cv::waitKey(1);


			// save rectangle for second eye
			eyeRoi2 = Rect(brightestPoint2.x - 50, brightestPoint2.y - 50, 100, 100);

			if (eyeRoi2.x > 0 && eyeRoi2.y > 0 && eyeRoi2.width > 0 && eyeRoi2.height > 0)
			{
				eyeRegions.push_back(eyeRoi2);
				// draw rectangle around
				rectangle(bright, eyeRoi2, Scalar(0, 0, 0), 1, 8, 0);
			}

			

		}
	}
	



// 4. Sort found regions

	// if two regions where found
	if(eyeRegions.size() >1)
	{
		// if roi1 is more left than roi2 switch order. Because right eye has smaller x than left eye.
		// subscript 0 musst contain right eye (smaller x value)
		// subscript 1 must contain left eye ( bigger x value)
		if (eyeRoi1.x > eyeRoi2.x)
		{
			Rect tmp = eyeRegions[0];
			eyeRegions[0] = eyeRegions[1];
			eyeRegions[1] = tmp;
		}
	}
	else
	{
		// only one region was found
		if (eyeRegions.size() == 1)
		{

			// if only one eye is visible, check if on left or right side of image. 
			// right half => left eye (becaused flipped)
			// left half => right eye 
			if (eyeRegions[0].x < APERTURE_WIDTH /2)
			{
				// found eye is right eye and at right spot in list
				// add empty second rect
				eyeRegions.push_back(Rect(0, 0, 0, 0));

			}
			else
			{
				// found eye with subscript 0 is left eye. move to subscrupt 1 and set 
				// empty rect on subscript 0
				Rect tmp = eyeRegions[0];
				eyeRegions[0] = Rect(0, 0, 0, 0);
				eyeRegions.push_back(tmp);
			}
		}
	}

	return eyeRegions;

}

// HAAR Cascade
// ========================================================================== DETECTION FUNCTIONS
vector<Rect> GoalkeeperAnalysis::getEyeRegion(cv::Mat frame)
{

	// Detect Eyes
	std::vector<Rect> eyes;
	Rect left, right;
	Point2d smallest = Point(1000, 1000);
	Point2d biggest = Point(0, 0);
	Mat resizedFrame;
	int resize_fact = 4;

	// resize image
	cv::resize(frame, resizedFrame, Size(int(frame.cols / resize_fact), int(frame.rows / resize_fact)), 0, 0, INTER_LINEAR);


	eye_cascade.detectMultiScale(resizedFrame, eyes, 1.1, 3, 0 | CV_HAAR_SCALE_IMAGE, Size(130/ resize_fact, 130/ resize_fact));


	// walk through all found eyes and draw rectangle
	for (size_t j = 0; j < eyes.size(); j++)
	{
		Point center(eyes[j].x + eyes[j].width*0.5, eyes[j].y + eyes[j].height*0.5);


		// get left most eye
		if(eyes[j].x < smallest.x && eyes[j].y < smallest.y)
		{
			smallest.x = eyes[j].x;
			smallest.y = eyes[j].y;
			left = eyes[j];
		}

		// get right most eye
		if (eyes[j].x > biggest.x && eyes[j].y > biggest.y)
		{
			biggest.x = eyes[j].x;
			biggest.y = eyes[j].y;
			right = eyes[j];
		}
	}

	
	if (resize_fact>0)
	{
	// resize back
	left.x = resize_fact*left.x;
	left.y = resize_fact*left.y;
	left.height = resize_fact*left.height;
	left.width = resize_fact*left.width;


	right.x = resize_fact*right.x;
	right.y = resize_fact*right.y;
	right.height = resize_fact*right.height;
	right.width = resize_fact*right.width;
	}
	

	
	vector<Rect> tmpVecEyes;
	tmpVecEyes.push_back(left);
	tmpVecEyes.push_back(right);

	int smaller = 60;
	//rectangle(frame, Point(left.x  + smaller, left.y + smaller), Point(left.x + left.width+smaller, left.y + left.height - smaller), Scalar(0, 0, 255), 1, 8, 0);
	//rectangle(frame, Point(right.x + smaller, right.y + smaller), Point(right.x + right.width - smaller, right.y + right.height - smaller), Scalar(0, 255, 0), 1, 8, 0);

	return tmpVecEyes;

}

// Function returns difference image of 2 frames
cv::Mat GoalkeeperAnalysis::getDifferenceImage(cv::Mat *frame1, cv::Mat *frame2, int threshold, boolean glint)
{
	// Can be optimized by filtering out the glint found in the corner of the eye
	// each glint musst be tested, if its mean value around it is high or low. because 
	// the mean value around a real glint is pretty dark as a glint is on the surface of the dark iris and pupil
	// fals glints in the eye corner are lying inside the white area of the eye and therefor do have a light mean value

	cv::Mat differenceImage;

	Mat workingFrame1, workingFrame2;

	frame1->copyTo(workingFrame1);

		if (workingFrame1.channels() >2)
		{
			cv::cvtColor(workingFrame1, workingFrame1, CV_BGR2GRAY);						 //change the color image to grayscale image
		}


		// Normalize frames, equalize histogramm
		cv::normalize(workingFrame1, workingFrame1, 0, 255, NORM_MINMAX, CV_8UC3);
		cv::equalizeHist(workingFrame1, workingFrame1);


		frame2->copyTo(workingFrame2);

		if (workingFrame2.channels() >2)
		{
			cv::cvtColor(workingFrame2, workingFrame2, CV_BGR2GRAY);						 //change the color image to grayscale image
		}

		cv::normalize(workingFrame2, workingFrame2, 0, 255, NORM_MINMAX, CV_8UC3);
		cv::equalizeHist(workingFrame2, workingFrame2);									//equalize the histogram

	

	cv::absdiff(workingFrame1, workingFrame2, differenceImage);
	cv::threshold(differenceImage, differenceImage, threshold, 255, THRESH_BINARY);


	//morphological opening (removes small objects from the foreground)
	cv::erode(differenceImage, differenceImage, getStructuringElement(MORPH_ELLIPSE, Size(1, 1)));
	cv::dilate(differenceImage, differenceImage, getStructuringElement(MORPH_ELLIPSE, Size(2, 2)));

	//imshow("Difference", differenceImage);
	return differenceImage;
}

std::vector<Point2d> GoalkeeperAnalysis::detectGlints(cv::Mat brightFrame, cv::Mat darkFrame, Point2d pBrightestPoint)
{

	Point brightestPoint = pBrightestPoint;
	vector<Point2d> glints;
	boolean searchLeft = false;
	byte glintCount = 1;

	if (showDetection)
	{
		qDebug() << "Detecting.... ";
		qDebug() << "Starting GLint: " << brightestPoint.x << brightestPoint.y;
	}


	//1. Get brightest Point in Eye Region
	// get differenceImage
	cv::Mat differenceImage2;
	differenceImage2 = getDifferenceImage(&brightFrame, &darkFrame, glintLower, true);
//	blur(differenceImage2, differenceImage2, Size(5, 5));

	// brightest point found?
	if (brightestPoint.x > 0 && brightestPoint.y > 0)
	{

		// add glint to vector and sort them!
		glints.push_back(brightestPoint);

		// 2. search glint to the right
		Point2d glintRight1 = searchInSubArea(brightestPoint, differenceImage2, false);

		// glint to the right found?
		if (glintRight1.y > 0 && glintRight1.x > 0)
		{
			glintRight1.x = glintRight1.x + brightestPoint.x + 3;
			glintRight1.y = glintRight1.y + brightestPoint.y - 5;	// 3
			if (showDetection) { qDebug() << "r1:" << glintRight1.x << glintRight1.y; }
			glints.push_back(glintRight1);
			++glintCount;


			// 3. search glint to the right
			Point2d glintRight2 = searchInSubArea(glintRight1, differenceImage2, false);

			// glint to the right found?
			if (glintRight2.x > 0 && glintRight2.y > 0)
			{
				glintRight2.x = glintRight2.x + glintRight1.x + 3;
				glintRight2.y = glintRight2.y + glintRight1.y - 5; // 3
				if (showDetection) { qDebug() << "r2:" << glintRight2.x << glintRight2.y; }
				glints.push_back(glintRight2);
				searchLeft = false;
				++glintCount;

			}
			else
			{
				if (showDetection){ qDebug() << "r2 not usable!" ;}

				searchLeft = true;
			}
		}
		else
		{
			if (showDetection) { qDebug() << "r1 not usable!" ; }
			searchLeft = true;
		}

		// searching glint to the left
		if (searchLeft)
		{
			// 2. search glint to the right
			Point2d glintLeft1 = searchInSubArea(brightestPoint, differenceImage2, true);

			// glint to the right found?
			if (glintLeft1.y > 0 && glintLeft1.x > 0)
			{
				glintLeft1.x = glintLeft1.x + brightestPoint.x - 14;
				glintLeft1.y = glintLeft1.y + brightestPoint.y - 5;
				if (showDetection) { qDebug() << "l1:" << glintLeft1.x << glintLeft1.y; }
				glints.push_back(glintLeft1);
				++glintCount;



				// 3. search glint to the right
				Point2d glintLeft2 = searchInSubArea(glintLeft1, differenceImage2, true);

				// glint to the right found?
				if (glintLeft2.x > 0 && glintLeft2.y > 0 && glintCount < 3)
				{
					glintLeft2.x = glintLeft2.x + glintLeft1.x - 14;
					glintLeft2.y = glintLeft2.y + glintLeft1.y - 5;
					if (showDetection) { qDebug() << "l2:" << glintLeft2.x << glintLeft2.y; }
					glints.push_back(glintLeft2);
				}
				else
				{
					if (showDetection) { qDebug() << "l2 not usable!"; }

				}
			}
			else
			{
				if (showDetection) { qDebug() << "l1 not usable!"; }

			}
		}

		if(showDetection)
			qDebug() << "END--------------------------------------------------" << endl;
	}
	else
	{
		if (showDetection) { qDebug() << "Could not find a brightest Point. Please check Sensitivity Value for Thresholdimage."; }
	}

	if (drawDetection)
	{
		for (int i = 0; i < glints.size(); i++)
		{
			if (i == 0)
			{
				circle(brightFrame, glints[i], 2, Scalar(0, 255, 0), 1, 8, 0);
				cv::drawMarker(brightFrame, glints[i], cv::Scalar(0, 255, 0), MARKER_CROSS, 5, 1);              // 1     0,0
			}
			else
			{
				circle(brightFrame, glints[i], 2, Scalar(0, 0, 255), 1, 8, 0);
				cv::drawMarker(brightFrame, glints[i], cv::Scalar(0, 0, 255), MARKER_CROSS, 5, 1);              // 1     0,0

			}

		}
	}

	return glints;



}


/*
void glintFaltung(Mat brightFrame)
{


	Mat filteredImg;
	Mat kernel;
	Mat dilated;
	brightFrame.copyTo(kernel);
	bitwise_not(brightFrame, kernel);


	// or, if the Mat_<float> is too weird for you, like this:
	//float kdata[] = { 1, 4, 6, -1, 3, 5, -1, -2, 2 };
	//Mat kernel(3, 3, CV_32F, kdata);
	filter2D(brightFrame, filteredImg, -1, kernel, cv::Point(-1, -1), 0, BORDER_REPLICATE);
	//filter2D(brightFrame, filteredImg, 10, kernel);

	erode(kernel, dilated, getStructuringElement(MORPH_RECT, Size(3, 3)));

	imshow("kernel", kernel);
	imshow("dilated", dilated);
	imshow("flitered", filteredImg);
	waitKey(1);





	std::vector<Point2d> glints;
	return 	glints;
}*/

Point2d GoalkeeperAnalysis::searchInSubArea(Point2d fatherPoint, Mat fatherArea, boolean left)
{
	boolean debug = true;
	Rect cut;
	Point2d glint = Point2d(0, 0);

	// if new searcharea is left from starting point get new rectangle with:
	// -3 pixel left from starting point
	// -10 px down, width= 11 height = 20
	// x_new => x_old - 14(offset+width)
	// y_new => y_old - half of height

	// if new searcharea is right from starting point get new rectangle with:
	// 3 pixel right from starting point
	// -10 px down, width= 11 height = 20 
	// x_new => x_old + 3(offset)
	// y_new => y_old - half of height
	if (left)
	{											// 10 20
		cut = Rect(fatherPoint.x - 14, fatherPoint.y-5 , 11, 10);

	}
	else
	{
		 cut = Rect(fatherPoint.x + 3, fatherPoint.y-5, 11, 10);

	}
	
	
	if(0 <= cut.x && 0 <= cut.width && cut.x + cut.width <= fatherArea.cols && 0 <= cut.y && 0 <= cut.height && cut.y + cut.height <= fatherArea.rows)
	{
		// Cut new searchArea rectangle from fatherMat
		Mat nextGlintArea = fatherArea(cut);

		if(nextGlintArea.cols >  0 && nextGlintArea.rows>0)
		{
		
			if (debug)
			{
				//qDebug() << " ST--------------------------";
			}
			glint = getBrightestPoint(nextGlintArea);

			if (glint.x == 0 && glint.y == 0)
			{
				return Point2d(0, 0);
			}
			else
			{
				return glint;
			}
	
		}
		

			
	}
	return Point2d(0, 0);


}

Point2d GoalkeeperAnalysis::getBrightestPoint(Mat frame)
{
	
	if (frame.channels() > 2)
	{
		cv::cvtColor(frame, frame, CV_BGR2GRAY);
	}

	double min, max;
	cv::Point min_loc, max_loc;
	cv::minMaxLoc(frame, &min, &max, &min_loc, &max_loc);

	//qDebug() << "    Min Brightness is:" << min;
	//qDebug() << "    Max Brightness is:" << max;
	//qDebug() << "    Max - Min  Brightness is:" << max - min;

		return max_loc;

}

GoalkeeperAnalysis::pupilGlintCombiInstance GoalkeeperAnalysis::detectGlintAndPupil(cv::Mat frame1, cv::Mat frame2)
{

	// save margins from cutted out regions in these variables
	Mat brightRightEye, brightLeftEye;
	Mat darkRightEye, darkLeftEye;
	Rect rightEyeRect, leftEyeRect;


	// Eye Image scaled
	Point2d brightestPointRight_onEyeScale = Point2d(rightEyeRect.x + 50, rightEyeRect.y + 50);
	Point2d brightestPointLeft_onEyeScale = Point2d(leftEyeRect.x + 50, leftEyeRect.y + 50);
	vector<Point2d> glintsLeft, glintsRight;
	Point2d pupilCenterLeft = Point(0, 0);
	Point2d pupilCenterRight = Point(0, 0);

	// Full image scaled
	Point2d brightestPointRight, brightestPointLeft;
	Point2d pupilCenterLeft_wholeScale, pupilCenterRight_wholeScale;
	vector<Point2d> glintsLeft_fullScale, glintsRight_fullScale;


	pupilGlintCombiInstance curPositions;




//0. detect bright/dark frame
//////////////////////////////////////////////////////////////////////////////////////////////
Scalar frame1Mean = mean(frame1);
Scalar  frame2Mean = mean(frame2);
Mat bright;
Mat dark;

if (frame1Mean.val[0] > frame2Mean.val[0])
{
	bright = frame1;
	dark = frame2;
}
else
{
	bright = frame2;
	dark = frame1;
}



// 1.HAAR: Detect Eyes and split image 
//////////////////////////////////////////////////////////////////////////////////////////////	
	/*
	if (!bright.empty() && !dark.empty())
	{
		vector<Rect> tmpEyes = getEyeRegion(bright);

		// 1. Get separate Regions of Frame for both eyes
		if (!tmpEyes.empty())
		{


			 brightRightEye = bright(tmpEyes[0]);
			 brightLeftEye = bright(tmpEyes[1]);

			 darkRightEye = dark(tmpEyes[0]);
			 darkLeftEye = dark(tmpEyes[1]);

			// 2. Save margin values of cutted out regions for later back calculating the position in the whole image
			// original postition of point in region is at x: leftEyeMarginTop + distance x in region itself
			//											   y: leftEyeMarginleft + distance y in region itself
			//											   x: rightEyeMarginTop + distance x in region itself
			//											   y: rightEyeMarginleft + distance y in region itself

			rightEyeMarginLeft = tmpEyes[0].x;
			rightEyeMarginTop = tmpEyes[0].y;

			leftEyeMarginLeft = tmpEyes[1].x;
			leftEyeMarginTop = tmpEyes[1].y;

		}
	}
	*/


	// 1.a Detect Eyes by brightest Point
	//////////////////////////////////////////////////////////////////////////////////////////////

if (!bright.empty() && !dark.empty())
{

	vector<Rect> eyeRegions = getEyeRegionsByBrightestPoints(bright, dark);
	eyeRegions.reserve(2);


	// 1. Get separate Regions of Frame for both eyes
	if (!eyeRegions.empty())
	{
		if (eyeRegions.size() == 2)
		{

			if ((0 <= eyeRegions[0].x && 0 <= eyeRegions[0].width && eyeRegions[0].x + eyeRegions[0].width <= bright.cols && 0 <= eyeRegions[0].y && 0 <= eyeRegions[0].height && eyeRegions[0].y + eyeRegions[0].height <= bright.rows))
			{
				brightRightEye = bright(eyeRegions[0]);
				darkRightEye = dark(eyeRegions[0]);

			}

			if ((0 <= eyeRegions[1].x && 0 <= eyeRegions[1].width && eyeRegions[1].x + eyeRegions[1].width <= bright.cols && 0 <= eyeRegions[1].y && 0 <= eyeRegions[1].height && eyeRegions[1].y + eyeRegions[1].height <= bright.rows))
			{

				brightLeftEye = bright(eyeRegions[1]);
				darkLeftEye = dark(eyeRegions[1]);

			}
			rightEyeRect = eyeRegions[0];
			leftEyeRect = eyeRegions[1];

		}
	}

}


// 2.  Glint Detection
//////////////////////////////////////////////////////////////////////////////////////////////
	
	if (!brightRightEye.empty() && !darkRightEye.empty())
	{
		glintsRight = detectGlints(brightRightEye, darkRightEye, brightestPointRight_onEyeScale);
		
		

		// Recalculate back to whole image
		for (int i = 0; i < glintsLeft.size(); i++)
		{
			if (glintsRight[i].x > 0 && glintsRight[i].y >0)
			{
				glintsRight[i] = Point2d(glintsRight[i].x + rightEyeRect.x, glintsRight[i].y + rightEyeRect.y);
			}
			else
			{
				glintsRight.erase(glintsRight.begin() + i);
			}
		}
	}
	
	
	if (!brightLeftEye.empty() && !darkLeftEye.empty())
	{
		glintsLeft = detectGlints(brightLeftEye, darkLeftEye, brightestPointLeft_onEyeScale);	
		// Recalculate back to whole image
		for (int i = 0; i < glintsLeft.size(); i++)
		{
			if (glintsLeft[i].x > 0 && glintsLeft[i].y >0)
			{
				glintsLeft[i] = Point2d(glintsLeft[i].x + leftEyeRect.x, glintsLeft[i].y + leftEyeRect.y);
			}
			else
			{
				glintsLeft.erase(glintsLeft.begin()+i);
			}
		}
		
	}	
	
	
// 3. b Else
	if (!darkRightEye.empty())
	{
		RotatedRect pupilRight = detectPupilELSE(brightRightEye, darkLeftEye);
		

		if (pupilRight.center.x > 0 && pupilRight.center.y > 0)
		{
			pupilCenterRight_wholeScale = Point2d(rightEyeRect.x + pupilRight.center.x, rightEyeRect.y + pupilRight.center.y);
		}
		else
		{
			pupilCenterRight_wholeScale = Point2d(0, 0);
		}
		qDebug() << "Pupil right: " << pupilRight.center.x << pupilRight.center.y;
		qDebug() << "Full Pupil right: " << pupilCenterRight_wholeScale.x << pupilCenterRight_wholeScale.y;



	}

	if (!darkLeftEye.empty() )
	{
		RotatedRect pupilLeft = detectPupilELSE(brightLeftEye,darkLeftEye);
		 
		if(pupilLeft.center.x > 0 && pupilLeft.center.y > 0)
		{
			pupilCenterLeft_wholeScale = Point2d(leftEyeRect.x + pupilLeft.center.x, leftEyeRect.y + pupilLeft.center.y);
		}
		else
		{
			pupilCenterLeft_wholeScale =Point2d(0,0);
		}

		qDebug() << "Pupil left: " << pupilLeft.center.x << pupilLeft.center.y;
		qDebug() << "Full Pupil left: " << pupilCenterLeft_wholeScale.x << pupilCenterLeft_wholeScale.y;


	}
	


	if (!brightLeftEye.empty())
	{
		imshow("LeftEye", brightLeftEye);

	}


	if (!brightRightEye.empty())
	{
		imshow("RightEye", brightRightEye);
	}

	// calculate and organize all glints and pupilcenters 
		curPositions = calcPupilGlintCombi(pupilCenterRight_wholeScale, pupilCenterLeft_wholeScale,glintsLeft, glintsRight, leftEyeRect.x, leftEyeRect.y, rightEyeRect.x, rightEyeRect.y);
		
	return curPositions;
}

/*

	Rechtes Auge:
	int contourEllipseSlider = 248; //70
	int minboxW = 20;
	int maxboxW = 97;
	beta = 1
	alpha = 3.0
*/
RotatedRect GoalkeeperAnalysis::fitEllipse_BHO(Mat img)
{

	// A ====================================================================================
	// optimizes pupil detection
	
	double alpha = 3.0;		// contrast		1.0-3.0
	int beta = 20;			// brightness	0  - 100

	if (img.channels() <3)
	{
		cv::cvtColor(img, img, CV_GRAY2BGR);
	}

	// Change brightness and contrast of eye image
	// Do  operation: new_image(i,j) = alpha*image(i,j) + beta
	for (int y = 0; y < img.rows; y++)
	{
		for (int x = 0; x < img.cols; x++)
		{
			for (int c = 0; c < 3; c++)
			{
				img.at<Vec3b>(y, x)[c] =
					saturate_cast<uchar>(alpha*(img.at<Vec3b>(y, x)[c]) + beta);
			}
		}
	}
	
	RotatedRect biggestBox;
	biggestBox.size.width = 2000;
	cv::cvtColor(img, img, CV_BGR2GRAY);
	
	vector<vector<Point> > contours;
	Mat bimage = img >= contourEllipseSlider;

	findContours(bimage, contours, RETR_LIST, CHAIN_APPROX_NONE);
	Mat cimage = Mat::zeros(bimage.size(), CV_8UC3);


	for (size_t i = 0; i < contours.size(); i++)
	{

		// NEEDED BUT WHY?
		size_t count = contours[i].size();
		if (count < 6)
			continue;


		Mat pointsf;
		Mat(contours[i]).convertTo(pointsf, CV_32F);
		RotatedRect box = fitEllipse(pointsf);
		if (box.size.width > minboxW && box.size.width < maxboxW && box.size.height)
		{
			if (box.size.width < biggestBox.size.width)
			{
				biggestBox = box;
				
			}
			

		}
	}

	return biggestBox;
}



// CHANGE TO INTERPOLATION
void GoalkeeperAnalysis::equalizeGlints(Mat *frame, vector<Point2d> glints)
{
	if (frame->channels() == 3)
	{
		cvtColor(*frame,*frame,CV_BGR2GRAY);
	}

	cv::Scalar mean = cv::mean(*frame);
	double min, max;
	Point min_loc, max_loc;
	cv::minMaxLoc(*frame, &min, &max, &min_loc, &max_loc);

	for (int i = 0; i < glints.size(); i++)
	{

		circle(*frame, glints[i], 3, min/*Scalar(0,0,0)*/, CV_FILLED, 8, 0);
	}
	if (frame->channels() != 3)
	{
		cvtColor(*frame, *frame, CV_GRAY2BGR);
	}
	
}

Point GoalkeeperAnalysis::getPupil(Mat src)

{

	
	if (src.channels() == 3)
	{
		cvtColor(src, src, CV_BGR2GRAY);

	}
	
	adaptiveThreshold(src, src, 255, ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 15, 3);
	//adaptiveThreshold(src, src, a1, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY,blockSize, c1);

	double min, max;
	cv::Point min_loc, max_loc;
	cv::minMaxLoc(src, &min, &max, &min_loc, &max_loc);


	//Find the contours. Use the contourOutput Mat so the original image doesn't get overwritten
	std::vector<std::vector<cv::Point> > contours;
	cv::Mat contourOutput = src.clone();
	cv::findContours(contourOutput, contours, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);

	//Draw the contours
	cv::Mat contourImage(src.size(), CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Scalar colors[3];
	colors[0] = cv::Scalar(255, 0, 0);
	colors[1] = cv::Scalar(0, 255, 0);
	colors[2] = cv::Scalar(0, 0, 255);
	for (size_t idx = 0; idx < contours.size(); idx++)
	{
		cv::drawContours(contourImage, contours, idx, colors[idx % 3]);
	}

	imshow("Frame3", contourImage);
	waitKey(1);
	return min_loc;
	
	
	/// Convert it to gray
	cvtColor(src, src, CV_BGR2GRAY);

	/// Reduce the noise so we avoid false circle detection
	GaussianBlur(src, src, Size(9, 9), 2, 2);

	vector<Vec3f> circles;

	/// Apply the Hough Transform to find the circles
	HoughCircles(src, circles, CV_HOUGH_GRADIENT, 2, 4, 30, 300);

	/// Draw the circles detected
	for (size_t i = 0; i < circles.size(); i++)
	{
		Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
		int radius = cvRound(circles[i][2]);
		// circle center
		circle(src, center, 3, Scalar(0, 255, 0), -1, 8, 0);
		// circle outline
		circle(src, center, radius, Scalar(0, 0, 255), 3, 8, 0);
	}

	imshow("Frame4", src);
	waitKey(1);
	//return Point(0,0);
}

GoalkeeperAnalysis::pupilGlintCombiInstance GoalkeeperAnalysis::calcPupilGlintCombi(Point2d pupilCenterRight, Point2d pupilCenterLeft, vector<Point2d> glintsLeft, vector<Point2d> glintsRight, int leftEyeRectX, int leftEyeRectY, int rightEyeRectX, int rightEyeRectY)
{

	//3.  COMPUTING Glints and Combination => EyeVec
	//////////////////////////////////////////////////////////////////////////////////////////////
	// calculate according to old pupil position which glints are visible and save all in 
	// structure curPositions (pupilGlintCombiInstance)
	boolean debug = true;
	boolean isValid = true;
	boolean isValid2 = true;

	pupilGlintCombiInstance curPositions;


// 1. Is pupil detected?
	if (pupilCenterRight.x <= 0 && pupilCenterRight.y <= 0)
	{
		isValid2 = false;
	}
	

	if (pupilCenterLeft.x <= 0 && pupilCenterLeft.y <= 0)
	{
		isValid2 = false;
	}



// 2. Is rectangle greate than 0? => detected
	if (rightEyeRectX <= 0 && rightEyeRectY <= 0)
	{
		isValid2 = false;
	}

	if (leftEyeRectX <= 0 && leftEyeRectY <= 0)
	{
		isValid2 = false;
	}






// 4. Sort glints
	sort(glintsLeft.begin(), glintsLeft.end(), mySort);
	sort(glintsRight.begin(), glintsRight.end(), mySort);


// Calculate glint positions according to old position

	switch (glintsLeft.size())
	{
		// If only one glint -> Head in extrem position.
	case 1:
		if (debug) { qDebug() << "Left: Only one glint detected."; }

		// Eye turned to left
		if (oldPupilCenterLeft.x < pupilCenterLeft.x)
		{

			curPositions.glintLeft_1 = glintsLeft[0];
		}
		else // Eye turned to right
		{
			curPositions.glintLeft_3 = glintsLeft[0];
		}
		break;

		// If only two glint -> slightly turned
	case 2:
		if (debug) { qDebug() << "Only two glint detected. Head slightly turned."; }
		if (oldPupilCenterLeft.x < pupilCenterLeft.x)
		{
			curPositions.glintLeft_1 = glintsLeft[0];
			curPositions.glintLeft_2 = glintsLeft[1];
		}
		else // Eye turned to right
		{
			curPositions.glintLeft_2 = glintsLeft[0];
			curPositions.glintLeft_3 = glintsLeft[1];
		}
		break;

		// If only 3 glint
	case 3:
		if (debug) { qDebug() << "3 Glints detected. Head is straight."; }
		curPositions.glintLeft_1 = glintsLeft[0];
		curPositions.glintLeft_2 = glintsLeft[1];
		curPositions.glintLeft_3 = glintsLeft[2];
		break;
	default:
		if (debug)
		{
			qDebug() << glintsLeft.size() << " found.";
		}
		break;
	}


	// right eye
	switch (glintsRight.size())
	{
		// If only one glint -> Head in extrem position.
	case 1:
		if (debug) { qDebug() << "Left: Only one glint detected."; }

		// Eye turned to Left
		if (oldPupilCenterRight.x < pupilCenterRight.x)
		{
			curPositions.glintRight_1 = glintsRight[0];
		}
		else // Eye turned to right
		{
			curPositions.glintRight_3 = glintsRight[0];
		}
		break;

		// If only two glint -> slightly turned
	case 2:
		if (debug) { qDebug() << "Only two glint detected. Head slightly turned."; }

		// Eye turned to Left
		if (oldPupilCenterRight.x < pupilCenterRight.x)
		{
			curPositions.glintRight_1 = glintsRight[0];
			curPositions.glintRight_2 = glintsRight[1];
		}
		else // Eye turned to right
		{
			curPositions.glintRight_2 = glintsRight[0];
			curPositions.glintRight_3 = glintsRight[1];
		}
		break;

		// If only 3 glint
	case 3:
		if (debug) { qDebug() << "3 Glints detected. Head is straight."; }
		curPositions.glintRight_1 = glintsRight[0];
		curPositions.glintRight_2 = glintsRight[1];
		curPositions.glintRight_3 = glintsRight[2];

		break;
	default:
		if (debug) { qDebug() << glintsRight.size() << " found."; }
		break;
	}

	curPositions.pupilLeft = pupilCenterLeft;
	curPositions.pupilRight = pupilCenterRight;

	oldPupilCenterLeft = pupilCenterLeft;
	oldPupilCenterRight = pupilCenterRight;



	// TESTING: ONLY Combinations where no point is zero are used => to overcome false values

	if (pupilCenterRight.x < 1 || pupilCenterRight.y < 1 || pupilCenterLeft.x < 1 || pupilCenterLeft.y < 1)
	{
		isValid = false;
	}

	// TESTING OUTPUT FOR CALIBRATIN PURPOSE ANNA

	qDebug() << "Pupil right:" << pupilCenterRight.x << pupilCenterRight.y;
	outputfile << "Pright" << pupilCenterRight.x << pupilCenterRight.y;


	if (glintsRight.size() >0)
	{
		outputfile << "G0:" << glintsRight[0].x << glintsRight[0].y;
		qDebug() << "G0:" << glintsRight[0].x << glintsRight[0].y;


		if (glintsRight[0].x < 1 || glintsRight[0].y < 1)
		{
			isValid = false;
		}
		
	}
	else
	{
		isValid = false;

	}

	if (glintsRight.size() >1)
	{
		outputfile << "G1:" << glintsRight[1].x << glintsRight[1].y;

		qDebug() << "G1:" << glintsRight[1].x << glintsRight[1].y;


		if (glintsRight[1].x < 1 || glintsRight[1].y < 1)
		{
			isValid = false;
		}
	}
	else
	{
		isValid = false;

	}

	if (glintsRight.size() >2)
	{
		outputfile << "G2:" << glintsRight[2].x << glintsRight[2].y;
		qDebug() << "G2:" << glintsRight[2].x << glintsRight[2].y;


		if (glintsRight[2].x < 1 || glintsRight[2].y < 1)
		{
			isValid = false;
		}
	}
	else
	{
		isValid = false;

	}


	outputfile << "Pupil Left:" << pupilCenterLeft.x << pupilCenterLeft.y;
	qDebug() << "Pupil Left:" << pupilCenterLeft.x << pupilCenterLeft.y;


	if (glintsLeft.size() >0)
	{
		outputfile << "G0:" << glintsLeft[0].x << glintsLeft[0].y;
		qDebug() << "G0:" << glintsLeft[0].x << glintsLeft[0].y;

		if (glintsLeft[0].x < 1 || glintsLeft[0].y < 1)
		{
			isValid = false;
		}
	}
	else
	{
		isValid = false;

	}

	if (glintsLeft.size() >1)
	{
		outputfile << "G1:" << glintsLeft[1].x << glintsLeft[1].y;
		qDebug() << "G1:" << glintsLeft[1].x << glintsLeft[1].y;


		if (glintsLeft[1].x < 1 || glintsLeft[1].y < 1)
		{
			isValid = false;
		}
	}
	else
	{
		isValid = false;

	}

	if (glintsLeft.size() >2)
	{
		outputfile << "G2:" << glintsLeft[2].x << glintsLeft[2].y;
		qDebug() << "G2:" << glintsLeft[2].x << glintsLeft[2].y;

		if (glintsLeft[2].x < 1 || glintsLeft[2].y < 1)
		{
			isValid = false;

		}
	}
	else
	{
		isValid = false;

	}

	outputfile << endl;
	if (!isValid2)
	{
		curPositions.valid = false;
	}
	return curPositions;
}

void GoalkeeperAnalysis::testDraw(pupilGlintCombiInstance cur, Mat *frame)
{
	/*
	circle(*frame, cur.glintLeft_1, 4, Scalar(255, 255, 255), 1, 8, 0);
	circle(*frame, cur.glintLeft_2, 4, Scalar(255, 255, 255), 1, 8, 0);
	circle(*frame, cur.glintLeft_3, 4, Scalar(255, 255, 255), 1, 8, 0);
	circle(*frame, cur.glintRight_1, 4, Scalar(255, 255, 255), 1, 8, 0);
	circle(*frame, cur.glintRight_2, 4, Scalar(255, 255, 255), 1, 8, 0);
	circle(*frame, cur.glintRight_3, 4, Scalar(255, 255, 255), 1, 8, 0);
	circle(*frame, cur.pupilLeft, 4, Scalar(0, 255, 0), 1, 8, 0);
	circle(*frame, cur.pupilRight, 4, Scalar(0, 0, 255), 1, 8, 0);
	
	cv::imshow("Frame1", *frame);*/
	
}

// Function takes image and detects pupil or iris and returns center of it
cv::Point2d GoalkeeperAnalysis::detectPupil_SimpleBlobDetector(cv::Mat src)
{

	
	// A ====================================================================================
	// optimizes pupil detection
	/*
	double alpha = 2.5;		// contrast		1.0-3.0
	int beta = 30;			// brightness	0  - 100

	if (src.channels() <3)
	{
		cv::cvtColor(src, src, CV_GRAY2BGR);
	}
	

	// Change brightness and contrast of eye image
	// Do  operation: new_image(i,j) = alpha*image(i,j) + beta
	for (int y = 0; y < src.rows; y++)
	{
		for (int x = 0; x < src.cols; x++)
		{
			for (int c = 0; c < 3; c++)
			{
				src.at<Vec3b>(y, x)[c] =
					saturate_cast<uchar>(alpha*(src.at<Vec3b>(y, x)[c]) + beta);
			}
		}
	}
	*/
	std::vector<KeyPoint> pupilKeypoints;


	// Detect circular blobs in left eye image
	pupilDetector->detect(src, pupilKeypoints);

	drawKeypoints(src, pupilKeypoints, src, Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

	if (pupilKeypoints.size() > 0)
	{
		return  Point2f(pupilKeypoints[0].pt.x, pupilKeypoints[0].pt.y);

	}
	else
	{
		return Point2f(0, 0);

	}



}

// ====================================================================================================== CALIBRATION AND MAPPING

Point2d GoalkeeperAnalysis::calculateEyeVector(Point2d pupil, Point2d glint1, Point2d glint2, Point2d glint3)
{
	Point2d tmp;
	tmp.x = abs(pupil.x + (glint1.x - glint2.x - glint3.x));
	tmp.y = abs(pupil.y + (glint1.y - glint2.y - glint3.y));
	return tmp;
}

// calculates mean values for all pupil centers and glints found on each calibration cross and saves them
void GoalkeeperAnalysis::getMeanValues(std::vector<pupilGlintCombiInstance> allPointsFoundOn, GoalkeeperAnalysis::calibrationPointCombination *tmpP)
{


	//long totalGlint0Left_x = 0;
	//long totalGlint0Left_y = 0;

	long fullGlintLeft_1X = 0;
	long fullGlintLeft_1Y = 0;

	long fullGlintLeft_2X = 0;
	long fullGlintLeft_2Y = 0;

	long fullGlintLeft_3X = 0;
	long fullGlintLeft_3Y = 0;

	long fullGlintRight_1X = 0;
	long fullGlintRight_1Y = 0;

	long fullGlintRight_2X = 0;
	long fullGlintRight_2Y = 0;

	long fullGlintRight_3X = 0;
	long fullGlintRight_3Y = 0;

	long fullPupilLeftX = 0;
	long fullPupilLeftY = 0;

	long fullPupilRightX = 0;
	long fullPupilRightY = 0;

	byte glintLeft1_ZeroCount = 0;
	byte glintLeft2_ZeroCount = 0;
	byte glintLeft3_ZeroCount = 0;

	byte glintRight1_ZeroCount = 0;
	byte glintRight2_ZeroCount = 0;
	byte glintRight3_ZeroCount = 0;

	byte pupilLeft_ZeroCount = 0;
	byte pupilRight_ZeroCount = 0;


	for (int i = 0; i < allPointsFoundOn.size(); i++)
	{
		
		allPointsFoundOn[i].glintLeft_1.x > 0 ? fullGlintLeft_1X = fullGlintLeft_1X + allPointsFoundOn[i].glintLeft_1.x : glintLeft1_ZeroCount++;
		fullGlintLeft_1Y = fullGlintLeft_1Y + allPointsFoundOn[i].glintLeft_1.y;

		allPointsFoundOn[i].glintLeft_2.x > 0 ? fullGlintLeft_2X = fullGlintLeft_2X + allPointsFoundOn[i].glintLeft_2.x : glintLeft2_ZeroCount++;
		fullGlintLeft_2Y = fullGlintLeft_2Y + allPointsFoundOn[i].glintLeft_2.y;

		allPointsFoundOn[i].glintLeft_3.x > 0 ? fullGlintLeft_3X = fullGlintLeft_3X + allPointsFoundOn[i].glintLeft_3.x : glintLeft3_ZeroCount++;
		fullGlintLeft_3Y = fullGlintLeft_3Y + allPointsFoundOn[i].glintLeft_3.y;

		allPointsFoundOn[i].glintRight_1.x > 0 ? fullGlintRight_1X = fullGlintRight_1X + allPointsFoundOn[i].glintRight_1.x : glintRight1_ZeroCount++;
		fullGlintRight_1Y = fullGlintRight_1Y + allPointsFoundOn[i].glintRight_1.y;

		allPointsFoundOn[i].glintRight_2.x > 0 ? fullGlintRight_2X = fullGlintRight_2X + allPointsFoundOn[i].glintRight_2.x : glintRight2_ZeroCount++;
		fullGlintRight_2Y = fullGlintRight_2Y + allPointsFoundOn[i].glintRight_2.y;

		allPointsFoundOn[i].glintRight_3.x > 0 ? fullGlintRight_3X = fullGlintRight_3X + allPointsFoundOn[i].glintRight_3.x : glintRight3_ZeroCount++;
		fullGlintRight_3Y = fullGlintRight_3Y + allPointsFoundOn[i].glintRight_3.y;

		allPointsFoundOn[i].pupilLeft.x > 0 ? fullPupilLeftX = fullPupilLeftX + allPointsFoundOn[i].pupilLeft.x : pupilLeft_ZeroCount++;
		fullPupilLeftY = fullPupilLeftY + allPointsFoundOn[i].pupilLeft.y;

		allPointsFoundOn[i].pupilRight.x > 0 ? fullPupilRightX = fullPupilRightX + allPointsFoundOn[i].pupilRight.x : pupilRight_ZeroCount++;
		fullPupilRightY = fullPupilRightY + allPointsFoundOn[i].pupilRight.y;
	}

	

	(allPointsFoundOn.size() - glintLeft1_ZeroCount) > 0 ? tmpP->glintLeft_1.x = fullGlintLeft_1X / (allPointsFoundOn.size() - glintLeft1_ZeroCount) : tmpP->glintLeft_1.x = fullGlintLeft_1X / 1;
	(allPointsFoundOn.size() - glintLeft1_ZeroCount) > 0 ? tmpP->glintLeft_1.y = fullGlintLeft_1Y / (allPointsFoundOn.size() - glintLeft1_ZeroCount) : tmpP->glintLeft_1.y = fullGlintLeft_1Y / 1;

	(allPointsFoundOn.size() - glintLeft2_ZeroCount) > 0 ? tmpP->glintLeft_2.x = fullGlintLeft_2X / (allPointsFoundOn.size() - glintLeft2_ZeroCount) : tmpP->glintLeft_2.x = fullGlintLeft_2X / 1;
	(allPointsFoundOn.size() - glintLeft2_ZeroCount) > 0 ? tmpP->glintLeft_2.y = fullGlintLeft_2Y / (allPointsFoundOn.size() - glintLeft2_ZeroCount) : tmpP->glintLeft_2.y = fullGlintLeft_2Y / 1;

	(allPointsFoundOn.size() - glintLeft3_ZeroCount) > 0 ? tmpP->glintLeft_3.x = fullGlintLeft_3X / (allPointsFoundOn.size() - glintLeft3_ZeroCount) : tmpP->glintLeft_3.x = fullGlintLeft_3X / 1;
	(allPointsFoundOn.size() - glintLeft3_ZeroCount) > 0 ? tmpP->glintLeft_3.y = fullGlintLeft_3Y / (allPointsFoundOn.size() - glintLeft3_ZeroCount) : tmpP->glintLeft_3.y = fullGlintLeft_3Y / 1;


	(allPointsFoundOn.size() - glintRight1_ZeroCount) > 0 ? tmpP->glintRight_1.x = fullGlintRight_1X / (allPointsFoundOn.size() - glintRight1_ZeroCount) : tmpP->glintRight_1.x = fullGlintRight_1X / 1;
	(allPointsFoundOn.size() - glintRight1_ZeroCount) > 0 ? tmpP->glintRight_1.y = fullGlintRight_1Y / (allPointsFoundOn.size() - glintRight1_ZeroCount) : tmpP->glintRight_1.y = fullGlintRight_1Y / 1;

	(allPointsFoundOn.size() - glintRight2_ZeroCount) > 0 ? tmpP->glintRight_2.x = fullGlintRight_2X / (allPointsFoundOn.size() - glintRight2_ZeroCount) : tmpP->glintRight_2.x = fullGlintRight_2X / 1;
	(allPointsFoundOn.size() - glintRight2_ZeroCount) > 0 ? tmpP->glintRight_2.y = fullGlintRight_2Y / (allPointsFoundOn.size() - glintRight2_ZeroCount) : tmpP->glintRight_2.y = fullGlintRight_2Y / 1;

	(allPointsFoundOn.size() - glintRight3_ZeroCount) > 0 ? tmpP->glintRight_3.x = fullGlintRight_3X / (allPointsFoundOn.size() - glintRight3_ZeroCount) : tmpP->glintRight_3.x = fullGlintRight_3X / 1;
	(allPointsFoundOn.size() - glintRight3_ZeroCount) > 0 ? tmpP->glintRight_3.y = fullGlintRight_3Y / (allPointsFoundOn.size() - glintRight3_ZeroCount) : tmpP->glintRight_3.y = fullGlintRight_3Y / 1;
	
	(allPointsFoundOn.size() - pupilLeft_ZeroCount) > 0 ? tmpP->pupilLeft.x = fullPupilLeftX / (allPointsFoundOn.size() - pupilLeft_ZeroCount) : tmpP->pupilLeft.x = fullPupilLeftX / 1;
	(allPointsFoundOn.size() - pupilLeft_ZeroCount) > 0 ? tmpP->pupilLeft.y = fullPupilLeftY / (allPointsFoundOn.size() - pupilLeft_ZeroCount) : tmpP->pupilLeft.y = fullPupilLeftY / 1;

	(allPointsFoundOn.size() - pupilRight_ZeroCount) > 0 ? tmpP->pupilRight.x = fullPupilRightX / (allPointsFoundOn.size() - pupilRight_ZeroCount) : tmpP->pupilRight.x = fullPupilRightX / 1;
	(allPointsFoundOn.size() - pupilRight_ZeroCount) > 0 ? tmpP->pupilRight.y = fullPupilRightY / (allPointsFoundOn.size() - pupilRight_ZeroCount) : tmpP->pupilRight.y = fullPupilRightY / 1;

	tmpP->toString();
}

// ====================================================================================================== UTILITIES

// shows calibration image and prints back calculated GAZE points on it
void GoalkeeperAnalysis::showResults()
{

	byte timeToShowEachCross = 3;

	// Initial Instruction window
	Mat bg(SCREEN_HEIGHT, SCREEN_WIDTH, CV_32F, cv::Scalar(255, 255, 255));
	cvtColor(bg, bg, CV_GRAY2BGR);

	putText(bg, "Results of calibration procedure ", Point2f(20, 20), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0, 0), 1);

	qDebug() << "Test Points blue:" << endl;
	for (int i = 0; i < testPoints.size(); i++)
	{
		qDebug() << "Point to show is" << testPoints[i].x << testPoints[i].y;
		circle(bg, testPoints[i], 10, Scalar(255, 0, 0), 3, 8, 0);
	}


	qDebug() << "Left green:" << endl;

	for (int i = 0; i < calcPoints1.size(); i++)
	{
		qDebug() << "Point to show is" << calcPoints1[i].x << calcPoints1[i].y;
		circle(bg, calcPoints1[i], 10, Scalar(0, 255, 0), 3, 8, 0);
	}

	qDebug() << "Right red:" << endl;

	for (int i = 0; i < calcPoints2.size(); i++)
	{
		qDebug() << "Point to show is" << calcPoints1[i].x << calcPoints1[i].y;
		circle(bg, calcPoints2[i], 10, Scalar(0, 0, 255), 3, 8, 0);
	}



	cv::drawMarker(bg, cv::Point2d(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 2), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);              // 1     0,0
	circle(bg, cv::Point2d(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 2), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	putText(bg, "1", Point2f((SCREEN_WIDTH / 10) + 20, (SCREEN_HEIGHT / 15 * 2) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point1.centerOfCrossOnScreen = cv::Point2d(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 2);
	point1.name = "Point 1";

	cv::drawMarker(bg, cv::Point2d(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 2), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 2     0,1
	circle(bg, cv::Point2d(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 2), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	putText(bg, "2", Point2f((SCREEN_WIDTH / 10 * 5) + 20, (SCREEN_HEIGHT / 15 * 2) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point2.centerOfCrossOnScreen = cv::Point2d(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 2);
	point2.name = "Point 2";

	cv::drawMarker(bg, cv::Point2d(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 2), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 3     0,2
	circle(bg, cv::Point(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 2), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	putText(bg, "3", Point2f((SCREEN_WIDTH / 10 * 9) + 20, (SCREEN_HEIGHT / 15 * 2) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point3.centerOfCrossOnScreen = cv::Point2d(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 2);
	point3.name = "Point 3";

	cv::drawMarker(bg, cv::Point2d(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 7), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);              // 4     1,0
	circle(bg, cv::Point2d(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 7), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	putText(bg, "4", Point2f((SCREEN_WIDTH / 10) + 20, (SCREEN_HEIGHT / 15 * 7) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point4.centerOfCrossOnScreen = cv::Point2d(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 7);
	point4.name = "Point 4";

	cv::drawMarker(bg, cv::Point2d(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 7), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 5     1,1 
	circle(bg, cv::Point2d(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 7), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	putText(bg, "5", Point2f((SCREEN_WIDTH / 10 * 5) + 20, (SCREEN_HEIGHT / 15 * 7) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point5.centerOfCrossOnScreen = cv::Point2d(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 7);
	point5.name = "Point 5";

	cv::drawMarker(bg, cv::Point2d(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 7), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);            // 6     1,2
	circle(bg, cv::Point2d(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 7), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	putText(bg, "6", Point2f((SCREEN_WIDTH / 10 * 9) + 20, (SCREEN_HEIGHT / 15 * 7) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point6.centerOfCrossOnScreen = cv::Point2d(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 7);
	point6.name = "Point 6";

	cv::drawMarker(bg, cv::Point2d(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 12), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);             // 7     2,0
	circle(bg, cv::Point2d(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 12), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	putText(bg, "7", Point2f((SCREEN_WIDTH / 10) + 20, (SCREEN_HEIGHT / 15 * 12) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point7.centerOfCrossOnScreen = cv::Point2d(SCREEN_WIDTH / 10, SCREEN_HEIGHT / 15 * 12);
	point7.name = "Point 7";

	cv::drawMarker(bg, cv::Point2d(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 12), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);           // 8     2,1
	circle(bg, cv::Point2d(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 12), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	putText(bg, "8", Point2f((SCREEN_WIDTH / 10 * 5) + 20, (SCREEN_HEIGHT / 15 * 12) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point8.centerOfCrossOnScreen = cv::Point2d(SCREEN_WIDTH / 10 * 5, SCREEN_HEIGHT / 15 * 12);
	point8.name = "Point 8";

	cv::drawMarker(bg, cv::Point2d(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 12), cv::Scalar(0, 0, 0), MARKER_CROSS, 40, 2);           // 9     2,2
	circle(bg, cv::Point2d(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 12), 20, cv::Scalar(0, 0, 0), 1, 8, 0);
	putText(bg, "9", Point2f((SCREEN_WIDTH / 10 * 9) + 20, (SCREEN_HEIGHT / 15 * 12) - 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0, 0), 1);
	point9.centerOfCrossOnScreen = cv::Point2d(SCREEN_WIDTH / 10 * 9, SCREEN_HEIGHT / 15 * 12);
	point9.name = "Point 9";

	imshow("Calibration", bg);
	waitKey();
}
// gets two differnet pictures and saves them pairwise
void GoalkeeperAnalysis::savePicture(int number, cv::Mat frame1, cv::Mat frame2)

{



	// save margins from cutted out regions in these variables
	int leftEyeMarginLeft, leftEyeMarginTop, rightEyeMarginLeft, rightEyeMarginTop;
	Mat brightRightEye, brightLeftEye;
	Mat darkRightEye, darkLeftEye;
	Point2d pupilCenterLeft = Point(0, 0);
	Point2d pupilCenterRight = Point(0, 0);
	pupilGlintCombiInstance curPositions;
	// all glints
	vector<Point2d> glintsLeft, glintsRight;


	//0. detect bright/dark frame
	//////////////////////////////////////////////////////////////////////////////////////////////

	Scalar frame1Mean = mean(frame1);
	Scalar  frame2Mean = mean(frame2);
	Mat bright;
	Mat dark;

	if (frame1Mean.val[0] > frame2Mean.val[0])
	{
		bright = frame1;
		dark = frame2;
	}
	else
	{
		bright = frame2;
		dark = frame1;
	}

	// 1.a Detect Eyes by brightest Point
	//////////////////////////////////////////////////////////////////////////////////////////////
	if (!bright.empty() && !dark.empty())
	{
		
		vector<Rect> eyeRegions = getEyeRegionsByBrightestPoints(frame1, frame2);
		
		eyeRegions.reserve(2);
		// 1. Get separate Regions of Frame for both eyes
		if (!eyeRegions.empty())
		{

			if (eyeRegions.size() > 0)
			{
				if ((0 <= eyeRegions[0].x && 0 <= eyeRegions[0].width && eyeRegions[0].x + eyeRegions[0].width <= bright.cols && 0 <= eyeRegions[0].y && 0 <= eyeRegions[0].height && eyeRegions[0].y + eyeRegions[0].height <= bright.rows))
				{


					brightRightEye = bright(eyeRegions[0]);
					darkRightEye = dark(eyeRegions[0]);

					//qDebug() << "Mat size: rows=" << brightRightEye.rows << "Cols=" << brightRightEye.cols;

					// 2. Save margin values of cutted out regions for later back calculating the position in the whole image
					// original postition of point in region is at x: leftEyeMarginTop + distance x in region itself
					//											   y: leftEyeMarginleft + distance y in region itself
					//											   x: rightEyeMarginTop + distance x in region itself
					//											   y: rightEyeMarginleft + distance y in region itself

					rightEyeMarginLeft = eyeRegions[0].x;
					rightEyeMarginTop = eyeRegions[0].y;
				}
			}

			if (eyeRegions.size() > 1)
			{

				if ((0 <= eyeRegions[1].x && 0 <= eyeRegions[1].width && eyeRegions[1].x + eyeRegions[1].width <= bright.cols && 0 <= eyeRegions[1].y && 0 <= eyeRegions[1].height && eyeRegions[1].y + eyeRegions[1].height <= bright.rows))
				{

					brightLeftEye = bright(eyeRegions[1]);
					darkLeftEye = dark(eyeRegions[1]);
					// 2. Save margin values of cutted out regions for later back calculating the position in the whole image
					// original postition of point in region is at x: leftEyeMarginTop + distance x in region itself
					//											   y: leftEyeMarginleft + distance y in region itself
					//											   x: rightEyeMarginTop + distance x in region itself
					//											   y: rightEyeMarginleft + distance y in region itself

					leftEyeMarginLeft = eyeRegions[1].x;
					leftEyeMarginTop = eyeRegions[1].y;


					//imshow("Frame1", brightRightEye);
					//imshow("Frame2", darkRightEye);
				}
			}
		}
	}




	ostringstream ss;
	ss << "brightImages/right" << number << ".png";
	string filename = ss.str();

	cv::imwrite(filename, brightRightEye);
	
	ostringstream ss2;
	ss2 << "brightImages/right" << number+1 << ".png";
	string filename2 = ss2.str();

	cv::imwrite(filename2, darkRightEye);

}

void GoalkeeperAnalysis::getSplit(vector<Point2d> glints, Rect &eyes)
{

	// calculate distance from each glint to all others
	// save only largest distance
	// after max is found => return leftGlint x + halb of distance of max

	double maxDistance = 0;
	double down = 600;
	double up = 0;
	double left = 800;
	double right = 0;
	double totalDistance = 0;
	double distance;

	for (int i = 0; i < glints.size(); ++i)
	{
		for (int j = 0; j < glints.size(); ++j)
		{
			distance = sqrt(pow((glints[i].x - glints[j].x), 2) + pow((glints[i].y - glints[j].y), 2));
			if (distance > maxDistance)
			{
				maxDistance = distance;
				if (glints[i].x < glints[j].x)
				{
					totalDistance = glints[i].x + (maxDistance / 2);
				}
				else
				{
					totalDistance = glints[j].x + (maxDistance / 2);
				}
			}
		}


		// check for lower bound
		if (glints[i].y < down)
		{
			down = glints[i].y;
		}
		else
		{
			// check for upper bound
			if (glints[i].y > up)
			{
				up = glints[i].y;
			}
		}

		// check for left bound
		if (glints[i].x < left)
		{
			left = glints[i].x;
		}
		else
		{
			// check for right bound
			if (glints[i].x > right)
			{
				right = glints[i].x;
			}
		}
	}

	int puffer = 50;


	// if left point is left enough, get bigger bounding box.
	// otherwise take left point as is
	if (left > puffer)
	{
		left -= puffer;
	}


	// if right point is far enough from end of image, get bigger bounding box.
	// otherwise take image end point
	if ((right + puffer) > 600)
	{
		right = 800;
	}
	else
	{
		right += puffer;
	}


	// if point is far away enough, get bigger bounding box
	// otherwise take limit of image 
	if (down > puffer)
	{
		down -= puffer;
	}


	if ((up + puffer) > 600)
	{
		up = 600;
	}
	else
	{
		up += puffer;
	}


	//Point p1(left,down);
	//Point p2(right,up);

	Point p1(0, down);
	Point p2(800, up);

	//qDebug() << "Rectanlge should be: " << p1.x << "/ " << p1.y << " and " << p2.x << "/" << p2.y;


	Rect eyes2(p1, p2);
	eyes = eyes2;

	// order according x-axis
	//sort(glints.begin(), glints.end(), sortByX);

}

double GoalkeeperAnalysis::getLengthOfVector(Point2d vecIn)
{
	double a = sqrt(pow(vecIn.x, 2) + pow(vecIn.y, 2));
	return a;
}


// TODO: find automatically the resolution of the second attached screen
void GoalkeeperAnalysis::getDesktopResolution(int& horizontal, int& vertical)
{
				//RECT desktop;
	// Get a handle to the desktop window
				//const HWND hDesktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
				//GetWindowRect(hDesktop, &desktop);
	// The top left corner will have coordinates (0,0)
	// and the bottom right corner will have coordinates
	// (horizontal, vertical)
			//horizontal = desktop.right;
			//vertical = desktop.bottom;

	horizontal = SCREEN_WIDTH;
	vertical = SCREEN_HEIGHT;

	cout << horizontal << '\n' << vertical << '\n';
}

// angle() 
// calculates the angle of the line from A to B with respect to the positive X-axis in degrees
int GoalkeeperAnalysis::angle(Point A, Point B)
{
	float y2 = B.y;
	float y1 = A.y;
	float x2 = B.x;
	float x1 = A.x;

	//Steigung ermitteln
	float m = (y2 - y1) / (x2 - x1);
	float angle = atan(m);
	angle = ((int)(angle * 180 / 3.14)) % 360;

	/*
	int val = (B.y - A.y) / (B.x - A.x); // calculate slope between the two points
	val = val - pow(val, 3) / 3 + pow(val, 5) / 5; // find arc tan of the slope using taylor series approximation
	val = ((int)(val * 180 / 3.14)) % 360; // Convert the angle in radians to degrees
	if (B.x < A.x) val += 180;
	if (val < 0) val = 360 + val;*/
	//qDebug() << "Angle: " << angle;
	return abs(angle);
}

// open last captured and saved video and try to detect glints and pupils
void GoalkeeperAnalysis::showPlayback()
{
	VideoCapture playback = VideoCapture::VideoCapture(currentVideo);
	double fps = playback.get(CV_CAP_PROP_FPS);
	int frameNumber = 0;
	qDebug() << fps;
	if (playback.isOpened())
	{
		Mat edges;
		namedWindow("edges", 1);
		Mat tmpFrame1;
		Mat tmpFrame2;
		Mat frame1;
		Mat frame2;

		//
		while (playback.get(CV_CAP_PROP_POS_FRAMES)<playback.get(CV_CAP_PROP_FRAME_COUNT) - 1)
		{

			playback >> tmpFrame1; // get a new frame from camera
			frame1 = tmpFrame1.clone();
			//imshow("Frame1", frame1);

			playback >> tmpFrame2; // get a new frame from camera
			frame2 = tmpFrame2.clone();

			if (false)
			{
				if (frame1.size > 0 && frame2.size > 0)
				{
					imshow("Frame1", frame1);
					imshow("Frame2", frame2);
				}
			}
			else
			{

				detectGlintAndPupil(frame1, frame2);
				frameNumber = frameNumber + 2;
			}
			cv::waitKey(30);


		}
	}
	else
	{

		qDebug() << "Could not open video!";
	}
	playback.release();
}

// function to compare two images for their brightness etc.
void GoalkeeperAnalysis::compare()
{
	Mat differenceImage;
	ostringstream ss;
	ostringstream ss2;
	ostringstream ss3;
	for (int i = 0; i < 500; i++)
	{
		ss << "C:/Testimages_Glint/test" << i << ".png";
		ss2 << "C:/Testimages_Glint/test" << i + 1 << ".png";

		Mat img1 = imread(ss.str());
		Mat img2 = imread(ss2.str());

		cv::absdiff(img1, img2, differenceImage);

		ss3 << "C:/DiffImages/diff" << i << ".png";
		string filename = ss3.str();
		vector<int> compression_params;
		compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
		compression_params.push_back(9);


		try
		{
			imwrite(filename.c_str(), differenceImage, compression_params);
		}
		catch (runtime_error& ex)
		{
			fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
		}


	}



}

// initializeVideoWriter()
// generate VideoWriter Object, opens up file to save frames into. setting codec, framerate etc
void GoalkeeperAnalysis::initializeVideoWriter()
{

// Video NAME
	//Timestamp erstellen
	long curTime = getCurTime();
	// create filename pattern: video_TIMESTAMP_COLORMODE.avi
	//const String fileName = "zvideo_" + std::to_string(curTime) + ".avi";

	const String fileName = userPath + "zvideo_" + std::to_string(curTime) + ".avi";
	//const String fileName ="zvideo_" + std::to_string(curTime) + ".avi";


// CODEC
	// bei -1 wird vom OS nach dem Codec gefragt. Eine Auswahl der installierten Codecs kann dann ausgewählt werden.
	//int codec = -1;

	// X works too but with bad quality
	// H is better choice
	// last parameter musst be true otherwise video cant be read 
	
	int codec = CV_FOURCC('H','2','6','4');

	// WOLFGANG
		//int codec = CV_FOURCC('H', 'F', 'Y','U');

// OUTPUT SIZE
	// IMPORTANT Size = WIDTH, HEIGHT
	// If Switched, output file will only have 6kb
	Size outputSize = Size(cam.sensorInfo.nMaxWidth, cam.sensorInfo.nMaxHeight);

	// WOLFGANG
		//Size outputSize = Size(100,100);

// OPEN WRITER AND FILE
	videoWriter = VideoWriter(fileName, codec, FPS, outputSize,true);
	// WOLFGANG
		//videoWriter = VideoWriter(fileName, codec, 25, outputSize, true);

	currentVideo = fileName;

	if(videoWriter.isOpened())
	{
		qDebug() << "VideoWriter opened";
	}
}

// get current Time
_int64 GoalkeeperAnalysis::getCurTime()
{
	__int64 now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	return now;
}

// destructor
GoalkeeperAnalysis::~GoalkeeperAnalysis()
{
	cam.closeCamera();
	cam.exit();
}

// On start or stop of capturing, activate or deactivate differnet gui elements
void GoalkeeperAnalysis::UpdateGUI()
{
	//ui.getLiveViewButton->setEnabled(!captureRunning);
	//ui.stopCaptureButton->setEnabled(captureRunning);
}

// show traffic sign
void MyFilledCircle(boolean detected)
{
	int thickness = -1;
	int lineType = 8;
	Mat traffic(200, 100, CV_8UC3, Scalar(0, 0, 0));
	Point greenCenter = Point(50, 50);
	Point redCenter = Point(50, 150);

	if (detected)
	{
		circle(traffic, greenCenter, 32.0, Scalar(0, 255, 0), thickness, lineType);
	}
	else
	{
		circle(traffic, redCenter, 32.0, Scalar(0, 0, 255), thickness, lineType);
	}

	namedWindow("Traffic", cv::WINDOW_KEEPRATIO | cv::WINDOW_AUTOSIZE);
	imshow("Traffic", traffic);

}


void GoalkeeperAnalysis::playVideo()
{
	VideoCapture playback = VideoCapture::VideoCapture(userFile);
	m_bIsCaptureRunning = true;

	// Get FPS of video
	double pb_fps = playback.get(CV_CAP_PROP_FPS);
	
	// 1000 ms / FPS => how long has to wait until show next frame
	double frameWait = 1000.0 / pb_fps;

	
	qDebug() << "Video Playback FPS are: " << pb_fps;

	int frameNumber = 0;
	Mat frame;
	Mat tmpFrame;


	if (playback.isOpened())
	{
		
		userVideoDictionary.startTime = getCurTime();								// set frame2time structure start time

		while (playback.get(CV_CAP_PROP_POS_FRAMES)<playback.get(CV_CAP_PROP_FRAME_COUNT) - 1 && m_bIsCaptureRunning)
		{

			playback >> frame;				// get a new frame from camera
			tmpFrame = frame.clone();		// clone frame 

			if (tmpFrame.size > 0)
			{
				userVideoDictionary.frame2time.emplace(playback.get(CV_CAP_PROP_POS_FRAMES), playback.get(CV_CAP_PROP_POS_MSEC));


				imshow("UserVideo", tmpFrame);
			}
			


			if (keyPressedReceiver->esc_pressed)
			{
				m_bIsCaptureRunning = false;
				break;
			}
			waitKey(1000.0 / pb_fps);//& 0xFF == 27)

		}	

		userVideoDictionary.endTime = getCurTime();								// set frame2time structure End time

	}
	else
	{

		qDebug() << "Could not open video!";
	}

	playback.release();
}


void GoalkeeperAnalysis::analyzeVideo()
{
	

// write eye video file
	eyeFile.open(userPath + "eyes.txt");

	qDebug() << "Eye Video Frames2Timestamp:";
	qDebug() << "Start: " << eyeVideoDictionary.startTime;

// analyze eye video

	// detect Glints and Pupil for each frame of eye video
	VideoCapture eyeVid = VideoCapture::VideoCapture(currentVideo);
	long frameCount = 0;
	Mat frame1, frame2;
	eyeFile << "Start: " << eyeVideoDictionary.startTime << endl;
	qDebug () << "Total amount of frames saved: " << eyeVid.get(CV_CAP_PROP_FRAME_COUNT);

	// walk though each frame find glints and pupil.
	// estimate gaze and save gaze in eyeVideoDictionary at right position according to frame number
	while (eyeVid.get(CV_CAP_PROP_POS_FRAMES)<eyeVid.get(CV_CAP_PROP_FRAME_COUNT) - 1)
	{
		qDebug() << "FrameCount: " << frameCount;
		eyeVid.set(CV_CAP_PROP_POS_FRAMES, frameCount);
		eyeVid.read(frame1);

		eyeVid.set(CV_CAP_PROP_POS_FRAMES, frameCount+1);
		eyeVid.read(frame2);

		frameCount += 2;
		pupilGlintCombiInstance pgci = detectGlintAndPupil(frame1,frame2);

		imshow("Frame1", frame1);
		imshow("Frame2", frame2);
		waitKey(1);


		// TODO: change to eyeVector with glints
		// saves frame number of eye Video and the calcualted gaze point
		gaze2frame.emplace(eyeVid.get(CV_CAP_PROP_POS_FRAMES), estimate(pgci.pupilRight));

		// save infos to file
		eyeFile << "Frame " << frameCount << ":  time " << eyeVideoDictionary.frame2time[frameCount] << " : Pupil_right " << pgci.pupilRight << " : Pupil_left " << pgci.pupilLeft << " : Glint_left_1 " << pgci.glintLeft_1 << " : Glint_left_2 " << pgci.glintLeft_2 << " : Glint_left_3 " << pgci.glintLeft_3 << " : Glint_right_1 " << pgci.glintRight_1 << " : Glint_right_2 " << pgci.glintRight_2 << " : Glint_right_3 " << pgci.glintRight_3 << endl;
		eyeFile << "Frame " << frameCount+1 << ":  time " << eyeVideoDictionary.frame2time[frameCount] << " : Pupil_right " << pgci.pupilRight << " : Pupil_left " << pgci.pupilLeft << " : Glint_left_1 " << pgci.glintLeft_1 << " : Glint_left_2 " << pgci.glintLeft_2 << " : Glint_left_3 " << pgci.glintLeft_3 << " : Glint_right_1 " << pgci.glintRight_1 << " : Glint_right_2 " << pgci.glintRight_2 << " : Glint_right_3 " << pgci.glintRight_3 << endl;

		 
	}

	
	eyeFile << "End: " << eyeVideoDictionary.endTime << endl;		

	qDebug() << "End: " << eyeVideoDictionary.endTime;

	// close file
	if (eyeFile.is_open())
	{
		eyeFile.close();
	}


	// save all of them in that eyeFile as csv or xml according to the frame
	// calculate gaze with estimate()
	// save gaze in eyesFileDictionary.gaze
	// get userFile and write on every frame the gaze point which saved during that time (look at eyeFileDictionary)



	// copy file to stimulus_with_gaze.avi
	// draw gaze points on it

}


void GoalkeeperAnalysis::drawGaze()
{

// TESTING PURPOSE
	for (int i = 0; i < 600; i++)
	{
		gaze2frame.emplace(i,Point2d(i+10,i+10));
	}

	VideoCapture stimulus = VideoCapture::VideoCapture(userFile);
	int userVideoWidth = stimulus.get(CV_CAP_PROP_FRAME_WIDTH);
	int userVideoHeight = stimulus.get(CV_CAP_PROP_FRAME_HEIGHT);

	int userVideoFPS = stimulus.get(CV_CAP_PROP_FPS);


	VideoWriter gazedVideoOut;
// draw gaze on each frame
	// 1. create new video file
	const String fileName2 = userPath + "output.avi";

	// last parameter musst be true otherwise video cant be read 
	int codec2 = CV_FOURCC('H', '2', '6', '4');
	//int codec2 = CV_FOURCC('H', 'F', 'Y','U');

// OUTPUT SIZE
	Size outputSize2 = Size(userVideoWidth, userVideoHeight);
	//Size outputSize2 = Size(userVideoHeight, userVideoWidth);

// OPEN WRITER AND FILE
	gazedVideoOut = VideoWriter(fileName2, codec2, userVideoFPS, outputSize2);


	if (videoWriter.isOpened())
	{
		qDebug() << "Stimulus video for gaze drawing opened";
	}


	int frameCount = 0;

	stimulus.set(CV_CAP_PROP_POS_FRAMES, frameCount);
	Mat img;
	long currentUserFrameTime, nextUserFrameTime;

	while (stimulus.get(CV_CAP_PROP_POS_FRAMES)<stimulus.get(CV_CAP_PROP_FRAME_COUNT) - 1)
	{
		// 1. read frame from user video
		stimulus.read(img);
		
		currentUserFrameTime = userVideoDictionary.frame2time[stimulus.get(CV_CAP_PROP_POS_FRAMES)];
		nextUserFrameTime = userVideoDictionary.frame2time[stimulus.get(CV_CAP_PROP_POS_FRAMES)+1];

		// 2. get corresponding gaze find all frames which timestamp is greater than the one from the user video but smaller than the NEXT timestamp of the uservideo frame timestamp
		for (int i = 0; i < eyeVideoDictionary.frame2time.size(); i++)
		{
			if (eyeVideoDictionary.frame2time[i] >= currentUserFrameTime && eyeVideoDictionary.frame2time[i] < nextUserFrameTime)
			{
				
				// 3. draw gaze on it
				cv::drawMarker(img, gaze2frame[i], cv::Scalar(255, 255, 255), MARKER_CROSS, 40, 2);       

				// 4. save frame in new file
				gazedVideoOut << img;

				qDebug() << "drawing gaze" << i;
				imshow("Frame1", img);
			}
		}
		
	}

	gazedVideoOut.release();


// show gaze



	
}

// ====================================================================================================== ELSE

cv::RotatedRect GoalkeeperAnalysis::detectPupilELSE(Mat frame1, Mat frame2)
{
	cv::RotatedRect pupil;
	pupil = ELSE::run(frame1, frame2,pupilSize);



	if (drawDetection)
	{
		cv::drawMarker(frame1, pupil.center, cv::Scalar(0, 255, 0), MARKER_CROSS, 10, 1);              // 1     0,0
	}
		return pupil;
}



void GoalkeeperAnalysis::testExcuse()
{
	int64  t1, t2, t3;
	float img_cnt_all = 0;
	t3 = 0;

	std::string path_data = "C:\\Goalkeeper\\GoalkeeperAnalysis\\GoalkeeperAnalysis\\images\\allImages\\";

	for (int kk = 0; kk<511; kk += 2)
	{
		
		std::stringstream ss;
		ss << std::setw(10) << std::setfill('0') << kk;
		std::string str = ss.str();

		std::string path_image = path_data + str + ".png";

		cv::Mat img_l1 = cv::imread(path_image, CV_LOAD_IMAGE_GRAYSCALE);

		if (!img_l1.data) std::cout << path_image << std::endl;



		std::stringstream ss2;
		ss2 << std::setw(10) << std::setfill('0') << kk + 1;
		std::string str2 = ss2.str();

		std::string path_image2 = path_data + str2 + ".png";

		cv::Mat img_l2 = cv::imread(path_image2, CV_LOAD_IMAGE_GRAYSCALE);

		if (!img_l2.data) std::cout << path_image2 << std::endl;

		t1 = cv::getTickCount();
		cv::RotatedRect pos = ELSE::run(img_l1, img_l2, pupilSize);
		t2 = cv::getTickCount();

		qDebug() << "Position is: " << pos.center;

		t3 += t2 - t1;


		img_cnt_all++;
	}

	printf("It took me %d clicks (%f seconds).\n", double(t3), ((double(t3)) / double(cv::getTickFrequency())) / img_cnt_all);

	std::cout << "ENDE" << std::endl;
	int tt;
	std::cin >> tt;

}


void imshowscale(const std::string& name, cv::Mat& m, double scale)
{
	cv::Mat res;
	cv::resize(m, res, cv::Size(), scale, scale, cv::INTER_NEAREST);
	cv::imshow(name, res);
}




void GoalkeeperAnalysis::changeGamma(int v)
{
	// do something with class members or v;
	cam.setGamma(v);
}

void GoalkeeperAnalysis::changeGain(int v)
{
	// do something with class members or v;
	cam.setGain(v);

}

void GoalkeeperAnalysis::changeHWGain(int v)
{
	// do something with class members or v;
	cam.setHWGain(v);

}

