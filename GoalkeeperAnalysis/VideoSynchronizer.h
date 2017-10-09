#ifndef VIDEOSYNCHRONIZER_H
#define VIDEOSYNCHRONIZER_H


#include <iostream> // for standard I/O
#include <string>   // for strings

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write


class VideoSynchronizer
{
	cv::VideoCapture userVideo;
	cv::VideoCapture eyeVideo;


};


#endif VIDEOSYNCHRONIZER_H