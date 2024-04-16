//Uncomment the following line if you are compiling this code in Visual Studio
//#include "stdafx.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;
using namespace std;

int main()
{

    
    //// 1. Create mask settings UI with default HSV range to detect blue color
    //auto const MASK_WINDOW = "Mask Settings";
   // cv::namedWindow(MASK_WINDOW);

    // HSV range to detect blue color
    int minHue = 0, maxHue = 5;
    int minSat = 0, maxSat = 255;
    int minVal = 0, maxVal = 255;

     //Create trackbars of mask settings window
    //cv::createTrackbar("Min Hue", MASK_WINDOW, &minHue, 179);
    //cv::createTrackbar("Max Hue", MASK_WINDOW, &maxHue, 179);
    //cv::createTrackbar("Min Sat", MASK_WINDOW, &minSat, 255);
    //cv::createTrackbar("Max Sat", MASK_WINDOW, &maxSat, 255);
    //cv::createTrackbar("Min Val", MASK_WINDOW, &minVal, 255);
    //cv::createTrackbar("Max Val", MASK_WINDOW, &maxVal, 255);

    while (true) {
        //// 2. Read and convert image to HSV color space
        cv::Mat inputImage{ cv::imread("C:/Users/PARHA/source/repos/OpenCVlearning/OpenCVlearning/IMG/redshoe.jpg", cv::IMREAD_COLOR) };
        //Mat inputImage = imread("C:/Users/PARHA/source/repos/OpenCVlearning/OpenCVlearning/IMG/red.png");

        cv::Mat inputImageHSV;
        cv::cvtColor(inputImage, inputImageHSV, cv::COLOR_BGR2HSV);

        //// 3. Create mask and result (masked) image
        cv::Mat mask;
        // params: input array, lower boundary array, upper boundary array, output array
        cv::inRange(
            inputImageHSV,
            cv::Scalar(minHue, minSat, minVal),
            cv::Scalar(maxHue, maxSat, maxVal),
            mask
        );
        cv::Mat resultImage;
        // params: src1	array, src2 array, output array, mask
        cv::bitwise_and(inputImage, inputImage, resultImage, mask);

        //// 4. Show images        
        cv::imshow("Input Image", inputImage);
        cv::imshow("Result (Masked) Image", resultImage);
        // imshow("Mask", mask);

        //// Wait for 'esc' (27) key press for 30ms. If pressed, end program.
        if (cv::waitKey(30) == 27) break;

       cv::imwrite("D:/shoe.jpg", resultImage);
    }
}