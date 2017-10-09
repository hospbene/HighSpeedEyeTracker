#include "utils.h"

/*
 * Global variables
 */

QElapsedTimer gTimer;

Timestamp maxTimestamp = std::numeric_limits<qint64>::max();

QString gExeDir;
QString gCfgDir;

char gDataSeparator = '\t';
char gDataNewline = '\n';

QMutex gLogMutex;
QFile gLogFile;
QTextStream gLogStream;

std::vector<QString> gLogBuffer;

/*
 * Utility functions
 */

QDebug operator<<(QDebug dbg, const cv::Point &p)
{
    dbg.nospace() << "(" << p.x << ", " << p.y << ")";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const cv::Point2f &p)
{
    dbg.nospace() << "(" << p.x << ", " << p.y << ")";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const cv::Point3f &p)
{
    dbg.nospace() << "(" << p.x << ", " << p.y << ", " << p.z << ")";
    return dbg.space();
}


void logExitBanner()
{
    QDateTime utc = QDateTime::currentDateTimeUtc();
    qDebug() << "Exiting\n######################################################################"
             << "\n# UTC:      " << utc.toString()
             << "\n# Local:    " << utc.toLocalTime().toString()
             << "\n######################################################################\n";

    std::cout.flush();
    gLogStream.flush();
}

cv::Point3f estimateMarkerCenter(const std::vector<cv::Point2f> corners)
{
    cv::Point3f cp(0,0,0);
    if (corners.size() != 4)
        return cp;

#ifdef HOMOGRAPHY_ESTIMATION
    // Homography is overkill here
    std::vector<cv::Point2f> plane;
    plane.push_back(cv::Point2f(0,0));
    plane.push_back(cv::Point2f(10,0));
    plane.push_back(cv::Point2f(10,10));
    plane.push_back(cv::Point2f(0,10));

    cv::Mat H = cv::findHomography(plane, corners);
    std::vector<cv::Point2f> planeCenter;
    planeCenter.push_back(cv::Point2f(5,5));
    std::vector<cv::Point2f> markerCenter;
    cv::perspectiveTransform(planeCenter, markerCenter, H);
    cp.x = markerCenter[0].x;
    cp.y = markerCenter[0].y;
#else
    double a1 = 0;
    double b1 = 0;
    double a2 = 0;
    double b2 = 0;

    if ( corners[2].x - corners[0].x != 0)
        a1 = (corners[2].y - corners[0].y) / (corners[2].x - corners[0].x);
    b1 = corners[2].y - corners[2].x*a1;

    if ( corners[3].x - corners[1].x != 0)
        a2 = (corners[3].y - corners[1].y) / (corners[3].x - corners[1].x);
    b2 = corners[3].y - corners[3].x*a2;

    cp.x = (b2-b1) / (a1-a2);
    cp.y = a2*cp.x + b2;
#endif

    return cp;
}

cv::Point2f to2D(const cv::Point3f p) { return cv::Point2f(p.x, p.y); }
cv::Point3f to3D(const cv::Point2f p) { return cv::Point3f(p.x, p.y, 0); }
