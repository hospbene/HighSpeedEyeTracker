#ifndef SYNCHRONIZER_H
#define SYNCHRONIZER_H

#include <list>

#include <QTimer>
#include <QObject>
#include <QSize>


#include "utils.h"

class InputData
{
public:
    // TODO: in the future we will have an enum for the type of data derived
    // (e.g., Left Eye, Right Eye) and make the synchronizer generic
    // (i.e., taking InputData and operating over a vector of these).
    // This would allow us to dynamically add input widgets
    InputData() {
        timestamp = maxTimestamp;
        processingTimestamp = maxTimestamp;
    }
    Timestamp timestamp;
    Timestamp processingTimestamp;
    virtual QString header(QString prefix) const = 0;
    virtual QString toQString() const = 0;
};

class Marker {
public:
    explicit Marker() :
        corners(std::vector<cv::Point2f>()),
        center(cv::Point3f(0,0,0)),
        id(-1),
        rv(cv::Mat()),
        tv(cv::Mat()) { }
    explicit Marker(std::vector<cv::Point2f> corners, int id) :
        corners(corners),
        center( cv::Point3f(0,0,0) ),
        id(id),
        rv(cv::Mat()),
        tv(cv::Mat()) { }
    std::vector<cv::Point2f> corners;
    cv::Point3f center;
    // Not exported atm
    int id;
    cv::Mat rv;
    cv::Mat tv;
};

class FieldData : public InputData {
public:
    explicit FieldData(){
        timestamp = 0;
        input = cv::Mat();
        gazeEstimate = cv::Point3f(0,0,0);
        gazeConfidence = 0.0;
        validGazeEstimate = false;
        extrapolatedGazeEstimate = 0;
        markers = std::vector<Marker>();
        collectionMarker = Marker();
        cameraMatrix = cv::Mat();
        distCoeffs = cv::Mat();
        undistorted = false;
        width = 0;
        height = 0;
        processingTimestamp = 0;
    }
    cv::Mat input;
    cv::Point3f gazeEstimate;
    bool validGazeEstimate;
    double gazeConfidence;
    int extrapolatedGazeEstimate;
    Marker collectionMarker;
    std::vector<Marker> markers;
    cv::Mat cameraMatrix, distCoeffs;
    bool undistorted;
    unsigned int width;
    unsigned int height;

    QString header(QString prefix = "") const {
        QString tmp;
        tmp.append(prefix + "timestamp");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "gaze.x");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "gaze.y");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "gaze.z");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "gaze.confidence");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "gaze.valid");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "collectionMarker.id");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "collectionMarker.x");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "collectionMarker.y");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "collectionMarker.z");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "undistorted");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "width");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "height");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "markers");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "processingTimestamp");
        tmp.append(gDataSeparator);
        return tmp;
    }

    QString toQString() const {
        QString tmp;
        tmp.append(QString::number(timestamp));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(gazeEstimate.x));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(gazeEstimate.y));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(gazeEstimate.z));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(gazeConfidence));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(validGazeEstimate));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(collectionMarker.id));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(collectionMarker.center.x));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(collectionMarker.center.y));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(collectionMarker.center.z));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(undistorted));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(width));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(height));
        tmp.append(gDataSeparator);
        for ( unsigned int i=0; i<markers.size(); i++) {
            tmp.append(QString::number(markers[i].id));
            tmp.append(":");
            tmp.append(QString::number(markers[i].center.x));
            tmp.append("x");
            tmp.append(QString::number(markers[i].center.y));
            tmp.append("x");
            tmp.append(QString::number(markers[i].center.z));
            tmp.append(";");
        }
        tmp.append(gDataSeparator);
        tmp.append(QString::number(processingTimestamp));
        tmp.append(gDataSeparator);
        return tmp;
    }
};

class EyeData : public InputData {
public:
    explicit EyeData(){
        timestamp = 0;
        input = cv::Mat();
        pupil = cv::RotatedRect(cv::Point2f(0,0), cv::Size2f(0,0), 0);
        confidence = 0.0;
        validPupil = false;
        eyeCenter = cv::Point2f(0,0);
        validEyeCenter = false;
        processingTimestamp = 0;
    }

    cv::Mat input;
    cv::RotatedRect pupil;
    double confidence;
    bool validPupil;
    cv::Point2f eyeCenter;
    bool validEyeCenter;

    // TODO: header, toQString, and the reading from file (see the Calibration class) should be unified
    // to avoid placing things in the wrong order / with the wrong string
    QString header(QString prefix = "") const {
        QString tmp;
        tmp.append(prefix + "timestamp");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.x");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.y");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.width");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.height");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.angle");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.valid");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.confidence");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "processingTimestamp");
        tmp.append(gDataSeparator);
        return tmp;
    }

    QString toQString() const {
        QString tmp;
        tmp.append(QString::number(timestamp));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(pupil.center.x));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(pupil.center.y));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(pupil.size.width));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(pupil.size.height));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(pupil.angle));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(validPupil));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(confidence));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(processingTimestamp));
        tmp.append(gDataSeparator);
        return tmp;
    }
};

class DataTuple {
public:
    DataTuple(){
        timestamp = maxTimestamp;
        lEye = EyeData();
        rEye = EyeData();
        field = FieldData();
        showGazeEstimationVisualization = false;
    }
    DataTuple(Timestamp timestamp, EyeData lEye, EyeData rEye, FieldData field)
        : timestamp(timestamp),
          lEye(lEye),
          rEye(rEye),
          field(field) {}
    Timestamp timestamp;
    EyeData lEye;
    EyeData rEye;
    FieldData field;
    cv::Mat gazeEstimationVisualization;
    bool showGazeEstimationVisualization;
    QString header() const {
        return QString("sync.timestamp") + gDataSeparator + FieldData().header("field.") + EyeData().header("left.") + EyeData().header("right.");
    }
    QString toQString() {
        return QString::number(timestamp) + gDataSeparator + field.toQString() + lEye.toQString() + rEye.toQString();
    }
};

class Synchronizer 
{

public:
    //explicit Synchronizer(QObject *parent = 0);
	Synchronizer();
	~Synchronizer();

signals:
    void newData(DataTuple dataTuple);

public:
    void newLeftEyeData(EyeData eyeData);
    void newRightEyeData(EyeData eyeData);
    void newFieldData(FieldData fieldData);
    void updateLists();

private:
    unsigned int maxAgeMs;
    unsigned int safeGuardMs;
    unsigned int maxListSize;
    std::list<EyeData> lEyeList;
    std::list<EyeData> rEyeList;
    std::list<FieldData> fieldList;
    bool updated;
    Timestamp lastSynchronization;

    QTimer *timer;
    template<class T> T* getClosestInTime(Timestamp timestamp, std::list<T> &dataList);
    //template<class T> Timestamp getLatestTimestamp(std::list<T> &dataList);
    //Timestamp getLatestTimestamp();

    template<class T> void removeOld(std::list<T> &dataList);
    void synchronize(Timestamp timestamp=0);
};

#endif // SYNCHRONIZER_H
