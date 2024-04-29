#include <opencv2/opencv.hpp>
#include "ObjectDetection.hpp"
#include "ObjectInfo.hpp"
using namespace cv;

int main() {
    std::string imgPath = "C:/Users/Sebastian WL/Desktop/Images/blood.jpg";

    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);

    ObjectDetection detection;

    ObjectInfo info = detection.findObjectInfo(image, 100, 100);

    // Print area and center points
    std::cout << "Area: " << info.area << std::endl;
    std::cout << "Center Point: (" << info.center.x << ", " << info.center.y << ")" << std::endl;

    // Display the image
    cv::imshow("Image with Contours", info.image);
    cv::waitKey(0); // Wait for a key press before closing the window

    return 0;
}

