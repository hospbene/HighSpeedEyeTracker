#include "GoalkeeperAnalysis.h"
#include <QtWidgets/QApplication>
#include <thread>


using namespace cv;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	GoalkeeperAnalysis w;
	w.show();
	return a.exec();
}


