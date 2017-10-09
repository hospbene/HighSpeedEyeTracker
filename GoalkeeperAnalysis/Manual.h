#ifndef MANUAL_H
#define MANUAL_H

#include <QString>
#include <QDebug>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui.hpp>

class Manual
{
public:
    Manual();
    ~Manual();

    cv::RotatedRect run(const cv::Mat &frame);
    bool hasPupilOutline() { return false; }
    std::string description() { return "Manual (experimental)"; }

private:
    cv::RotatedRect pupil;
    bool initializing;
    int manualThreshold;
    cv::Mat input;
    cv::Mat thresholded;
    cv::Mat dbg;
    cv::Point2f centroid;

    void init();
    void showThresholded();
    void thresholdImage();
    void removeNoise();
    cv::Point2f calculateCentroid();
    cv::RotatedRect ellipseFit();
    void autoThreshold();
    double calcMeanHorizontalPosition(std::vector<cv::Point> v) {
        double mean_x = 0;
        for (int i=0; i<v.size(); i++)
            mean_x += v[i].x;
        return mean_x / v.size();
    }
    bool merge(std::vector<cv::Point> ref, std::vector<cv::Point> candidate) {
        int refMin = INT_MAX;
        int refMax = INT_MIN;
        for (int i=0; i<ref.size(); i++) {
            if (ref[i].x < refMin)
                refMin = ref[i].x;
            if (ref[i].x > refMax)
                refMax = ref[i].x;
        }
        int candidateMin = INT_MAX;
        int candidateMax = INT_MIN;
        for (int i=0; i<candidate.size(); i++) {
            if (candidate[i].x < candidateMin)
                candidateMin = candidate[i].x;
            if (candidate[i].x > candidateMax)
                candidateMax = candidate[i].x;
        }

        if (candidateMin > refMax || candidateMax < refMin)
            return false;

        int s = std::max<int>(refMin, candidateMin);
        int e = std::min<int>(refMax, candidateMax);
        double candidateOverlapRatio = (e - s) / (double) (candidateMax - candidateMin);
        if ( candidateOverlapRatio > 0.5)
            return true;
        else
            return false;
    }

    bool showDbg;
};

#endif // MANUAL_H
