#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>

namespace TIBA
{
	

	cv::Mat conv2D(const cv::Mat& input, const cv::Mat& kernel)
	{
		cv::Mat flipped_kernel;
		flip(kernel, flipped_kernel, -1);

		cv::Point2i pad;
		cv::Mat result, padded;

		padded = input;
		pad = cv::Point2i(0, 0);

		cv::Rect region = cv::Rect(pad.x / 2, pad.y / 2, padded.cols - pad.x, padded.rows - pad.y);
		filter2D(padded, result, -1, flipped_kernel, cv::Point(-1, -1), 0, cv::BORDER_CONSTANT);

		return result(region);
	}

	static cv::Mat norm01(cv::Mat pic)
	{
		cv::Mat doublePic;
		pic.convertTo(doublePic, CV_64F);

		// get brightest and darkest point
		double min, max;
		cv::minMaxLoc(pic, &min, &max);

		for (int i = 0; i < doublePic.rows; i++)
		{
			for (int j = 0; j < doublePic.cols; j++)
			{
				doublePic.at<double>(i, j) = (
												(doublePic.at<double>(i, j)-min)
												/ 
												(max-min)
											);
			}
		}

		return doublePic;
	}



	static cv::Point2d gradcentervote(cv::Mat img)
	{


		cv::Point2d center = cv::Point2d(-1, -1);
		int x = img.rows;
		int y = img.cols;

		img = norm01(img);

		//gau = fspecial('gaussian', [5 5], 0.5);
		cv::Mat gauss = getGaussianKernel(5, 0.5, img.type());

		// img=norm01(conv2(img,gau,'same'));
	
		filter2D(img, img, -1, gauss, Point(-1, -1), 0.0 /*5.0*/, BORDER_CONSTANT);
		img = norm01(img);


		//sob_y = [1 0 - 1; 2 0 - 2; 1 0 - 1]; %matlab conv2d rotation
		cv::Mat sob_y = (cv::Mat_<double>(3, 3) << 1,0,-1,
												   2,0,-2,
												   1,0,-1 );


		//sob_x = [1 2 1; 0 0 0; -1 - 2 - 1]; %matlab conv2d rotation
		cv::Mat sob_x = (cv::Mat_<double>(3, 3) << 1, 1, 1,
												   0, 0, 0,
												   -1, -2, -1);


		cv::Mat g_x(img.rows, img.cols, img.type());
		cv::Mat g_y(img.rows, img.cols, img.type());


		//g_y = conv2(img, sob_y, 'same');
		filter2D(img, g_y, -1, sob_y, Point(-1, -1), 0.0 /*5.0*/, BORDER_CONSTANT);

	
		//g_x = conv2(img, sob_x, 'same');
		filter2D(img, g_x, -1, sob_x, Point(-1, -1), 0.0 /*5.0*/, BORDER_CONSTANT);
	
		cv::Mat g_s;

		//g_s = sqrt((g_x. ^ 2) + (g_y. ^ 2));
		magnitude(g_x, g_y, g_s);


		//g_s = g_s + ((g_s == 0).*0.0000001);
		for (int i = 0; i < g_x.rows; i++)
		{
			for (int j = 0; j < g_x.cols; j++)
			{
				if (g_s.at<double>(i, j) == 0.0)
				{
					g_s.at<double>(i, j) = 0.0000001;
				}
			
			}
		}


		// Matrix.* Matrix <=  elemnt-wise operation instead of matrix multiplication
		//g_x = g_x. / g_s;
			cv::divide(g_s, g_x, g_x);
		//g_y = g_y. / g_s;
			cv::divide(g_s, g_y, g_y);
	
		//votes = double(zeros(x, y));
			cv::Mat votes = cv::Mat::zeros(x,y,CV_64F);			

			double dist;
			double dy, dx;

			for(int i= 0; i < x; i++)
			{
				for (int j = 0; j < y; j++)
				{

					for(int i2 = 0; i2 < x;i2++)
					{
						for(int j2 = 0; j2 < y; j2++)
						{

							dist = sqrt(pow((i2 - i),2) + pow((j2 - j),2));
							if (dist > 0)
							{
								dy = (i2 - i) / dist;
								dx = (j2 - j) / dist;

								votes.at<double>(i, j) = votes.at<double>(i, j) + ((1.0 - img.at<double>(i, j)) * pow((dx*g_x.at<double>(i2, j2) + dy*g_y.at<double>(i2, j2)),2));
							}
						}
					}
				}
			}


			double max_val = 0;
			double center_x = -1;
			double center_y = -1;

			for(int i = 0;i<x;i++)
			{
				for(int j = 0; j< y;j++)
				{

					if( max_val < votes.at<double>(i, j))
					{
						max_val = votes.at<double>(i, j);
						center_x = j;
						center_y = i;
					}
				}
			}
			
			center.x = center_x;
			center.y = center_y;

		
		
			cv::drawMarker(img, center, cv::Scalar(255, 255, 255), MARKER_CROSS, 10, 1);
			cv::imshow("img", img);
			waitKey(0);
		

			return center;

	}














	static cv::Point2d run(cv::Mat img)
	{

		cv::Mat input = cv::imread("ex4.png");

		// get grayscale image
		if (input.channels() == 3)
		{
			cvtColor(input, input, CV_RGB2GRAY);
		}

		double scaley = input.rows / 40.0;
		double scalex = input.cols / 40.0;

		cv::Size a = input.size();
		int imgW = a.width;
		int imgH = a.height;
		qDebug() << "Widht: " << imgW << "height:" << imgH;


		// resize for better performance
		cv::Mat inputSmall;
		cv::resize(input, inputSmall, cv::Size(40, 40));

		cv::Point2d center = gradcentervote(inputSmall);

		center.x = center.x * scalex;
		center.y = center.y * scaley;

		/*
		cv::drawMarker(input, center, cv::Scalar(255, 255, 255), MARKER_CROSS, 10, 1);
		cv::imshow("img", input);
		waitKey(0);
		*/
		return center;

	}
}
