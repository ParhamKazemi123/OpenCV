#ifndef OBJECTDETECTION_HPP
#define OBJECTDETECTION_HPP

#include <opencv2/opencv.hpp>
#include <iostream>
#include "ObjectInfo.hpp"
using namespace cv;

class ObjectDetection {
public:
    cv::Mat identifyCenterObject(cv::Mat image);
    int identifyCenterObjectArea(cv::Mat image);
    std::string findCenterOfObject(cv::Mat image);

    cv::Mat findObject(cv::Mat image, int x, int y);
    int findObjectArea(cv::Mat image, int x, int y);

    ObjectInfo findObjectInfo(cv::Mat image, int x, int y);
    ObjectInfo CenterObjectInfo(cv::Mat image);

private:
    cv::Scalar contourColor = cv::Scalar(222, 181, 255);
    void drawWeightedContour(cv::Mat image, std::vector<cv::Point> contour);
    std::vector<std::vector<cv::Point>> getContours(cv::Mat& image);
};

#endif // OBJECTDETECTION_HPP