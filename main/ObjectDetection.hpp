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

private:
    std::vector<std::vector<cv::Point>> getContours(cv::Mat& image);
};

#endif // OBJECTDETECTION_HPP