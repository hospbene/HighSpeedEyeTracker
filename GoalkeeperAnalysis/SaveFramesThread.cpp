/*#include <SaveFramesThread.h>

// --- CONSTRUCTOR ---
SaveFramesThread::SaveFramesThread()
{
	// you could copy data from constructor arguments to internal variables here.
}




// --- DECONSTRUCTOR ---
SaveFramesThread::~SaveFramesThread()
{
	// free resources
}

// --- PROCESS ---
// Start processing data.
void SaveFramesThread::process()
{
	if (!framesQueue.empty())
	{
		qDebug() << "writing to file";
		try
		{
			sharedMutex.lock();
			videoWriter.write(framesQueue.front());
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
	emit finished();
}*/