#include <opencv2/opencv.hpp>
#include "ObjectDetection.hpp"
using namespace cv;

int main() {
    std::string imgPath = "C:/Users/Sebastian WL/Desktop/Images/blood.jpg";

    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);

    ObjectDetection detection;

    detection.centerObjectInfo(image);

    //Print area and center points
    std::cout << "Area: " << detection.getArea() << std::endl;
    std::cout << "Center Point: (" << detection.getCenter().x << ", " << detection.getCenter().y << ")" << std::endl;

    // Display the image
    cv::imshow("Image with Contours", detection.getImage());
    cv::waitKey(0); // Wait for a key press before closing the window

    return 0;
}

