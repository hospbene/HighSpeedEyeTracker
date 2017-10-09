#include "Manual.h"

#include <climits>
#include <iostream>
#include <QDebug>
#include <QElapsedTimer>

#define M_PI 3.14f
using namespace std;
using namespace cv;


Manual::Manual() :
    initializing(true),
    manualThreshold(60)
{
}

Manual::~Manual()
{
}

void Manual::autoThreshold()
{
    int amountTh = 0.01*input.rows*input.cols;
    int histogram[256] = {0};
    int lowTh = 255;

    uchar *p;
    for (int i=0; i<input.rows; i++) {
        p = input.ptr<uchar>(i);
        for(int j=0; j<input.cols; j++)
            histogram[p[j]]++;
    }

    vector<int> peaks;
    for (int i=0; i<256; i++)
        if (histogram[i] > amountTh)
            peaks.push_back(i);

    if (peaks.size() > 0)
        lowTh = peaks[0];
    else
        exit(1);

    manualThreshold = lowTh;
}

void Manual::showThresholded()
{
    Mat bgr;
    cvtColor(input, bgr, CV_GRAY2BGR);
    bgr.setTo(Scalar(0,255,0), thresholded);
    line(bgr, Point(pupil.center.x,0), Point(pupil.center.x, bgr.rows), Scalar(0,0,255));
    line(bgr, Point(0, pupil.center.y), Point(bgr.cols, pupil.center.y), Scalar(0,0,255));
    ellipse(bgr, pupil, Scalar(0,0,255));
    line(bgr, Point(centroid.x,0), Point(centroid.x, bgr.rows), Scalar(0,255,255));
    line(bgr, Point(0, centroid.y), Point(bgr.cols, centroid.y), Scalar(0,255,255));
    imshow("thresholded", bgr);
}

void Manual::thresholdImage()
{
    cv::inRange(input, 0, manualThreshold, thresholded);
    removeNoise();
}

void Manual::removeNoise()
{
#ifdef MORPH
    double perc = 0.025;
    int w = perc*thresholded.cols;
    int h = perc*thresholded.rows;
    Mat el = getStructuringElement(cv::MORPH_ELLIPSE, Size(w, h));
    morphologyEx(thresholded, thresholded, cv::MORPH_DILATE, el);
    dbg.setTo(Scalar(0,255,0), thresholded);
#else
    GaussianBlur(thresholded, thresholded, Size(7,7), 1.5);
#endif
}

void Manual::init()
{
    showDbg = true;
    autoThreshold();
    while (true) {
		if (input.channels() < 3)
		{
			cvtColor(input, dbg, CV_GRAY2BGR);
		}
		else
		{
			input.copyTo(dbg);
		}
        thresholdImage();
        ellipseFit();
        imshow("dbg", dbg);
        char key = cv::waitKey(0);
        switch (key) {
            case 'q':
                qDebug() << "quit";
                exit(1);
                break;
            case 'j':
                manualThreshold--;
                break;
            case 'k':
                manualThreshold++;
                break;
            case 'r':
                initializing = false;
                qDebug() << "ready";
                return;
            case 'u':
                qDebug() << "update";
                return;
            default:
                break;
        }
    }
    showDbg = false;
}

Point2f Manual::calculateCentroid()
{
    Moments m = moments(thresholded, true);
    if (m.m00 > 0)
        centroid = Point2f( m.m10 / m.m00, m.m01 / m.m00 );
    else
        centroid = Point2f( m.m10, m.m01 );
    line(dbg, Point(centroid.x,0), Point(centroid.x, dbg.rows), Scalar(0,255,255));
    line(dbg, Point(0, centroid.y), Point(dbg.cols, centroid.y), Scalar(0,255,255));
    return centroid;
}

cv::RotatedRect Manual::ellipseFit()
{
    Mat copy = thresholded.clone();

    vector<vector<Point> > contours;
    findContours(copy, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

    vector<vector<Point> > inliers, outliers;
    vector<double> areas;
    double largestArea = 0;
    int largestIdx = -1;
    for (unsigned int i=0; i<contours.size(); i++) {
        areas.push_back(contourArea(contours[i], false));
        if (areas.back() > largestArea) {
            largestArea = areas.back();
            largestIdx = i;
        }
    }
#ifdef LARGEST
    for (unsigned int i=contours.size(); i-->0;) {
        if ( areas[i] < 0.25*largestArea)
            outliers.push_back(contours[i]);
        else
            inliers.push_back(contours[i]);
    }
#else
    for (unsigned int i=contours.size(); i-->0;)
        if ( areas[i] / thresholded.total() < 0.001 )
            outliers.push_back(contours[i]);
        else
            inliers.push_back(contours[i]);
#endif

    if( showDbg ) {
        drawContours(thresholded, outliers, -1, 0, CV_FILLED);
        dbg.setTo(Scalar(0,255,0), thresholded);

        Mat tmp = input.clone();
        drawContours(tmp, inliers, -1, Scalar(255));
        imshow("contours", tmp);
    }

    vector<Point> all;
#ifdef USE_ALL
    for (unsigned int i=0; i<inliers.size(); i++)
        all.insert( all.end(), inliers[i].begin(), inliers[i].end());
#else
    if (largestIdx == -1) {
        pupil = RotatedRect( Point2f(-1,-1), Size2f(-1,-1), -1);
        return pupil;
    }

    all = contours[largestIdx];
    double ref = calcMeanHorizontalPosition(all);
    for (unsigned int i=0; i<inliers.size(); i++)
        if ( merge(all, inliers[i]) && abs( calcMeanHorizontalPosition(inliers[i]) - ref ) < 0.1*input.cols )
            all.insert( all.end(), inliers[i].begin(), inliers[i].end());
#endif

    if (all.size() < 5) {
        pupil = RotatedRect( Point2f(-1,-1), Size2f(-1,-1), -1);
        return pupil;
    }

    vector<Point> hull;
    convexHull(all, hull);
    if ( hull.size() > 5) {
        pupil = fitEllipse(hull);
        if (showDbg) {
            line(dbg, Point(pupil.center.x,0), Point(pupil.center.x, dbg.rows), Scalar(0,0,255));
            line(dbg, Point(0, pupil.center.y), Point(dbg.cols, pupil.center.y), Scalar(0,0,255));
            ellipse(dbg, pupil, Scalar(0,0,255));
        }
    }

    calculateCentroid();
    return pupil;
}

RotatedRect Manual::run(const Mat &frame)
{
    QElapsedTimer timer;
    timer.start();

    input = frame;
    pupil = RotatedRect( Point2f(-1,-1), Size2f(-1,-1), -1);
    if (showDbg)
        cvtColor(input, dbg, CV_GRAY2BGR);

    if (initializing) {
        init();
        qDebug() << "Manual Threshold:" << manualThreshold;
        return pupil;
    }

    thresholdImage();
    ellipseFit();

    if (showDbg)
        imshow("dbg", dbg);

    qDebug() << timer.elapsed();
    //// Normalize first
    //Mat normalized;
    //normalize(frame, normalized, 0, 255, NORM_MINMAX, CV_8U);

    return pupil;
}
