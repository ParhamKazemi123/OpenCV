#include <opencv2/opencv.hpp>
#include <iostream>
using namespace cv;

// Global vector to store contours
std::vector<std::vector<cv::Point>> contoursList;

// Function to add a contour to the list if it's not a duplicate
void addContour(const std::vector<cv::Point>& contour) {
    // Check for duplicate contours
    if (std::find(contoursList.begin(), contoursList.end(), contour) == contoursList.end()) {
        contoursList.push_back(contour);
    }
}

int contourThickness(cv::Mat image) {
    int rows = image.rows;
    int cols = image.cols;

    int thickness = 2;

    thickness += (rows + cols) / 200;

    return thickness;
}

cv::Mat padImage(const cv::Mat& src, int padSize) {
    cv::Mat padded;
    cv::copyMakeBorder(src, padded, padSize, padSize, padSize, padSize, cv::BORDER_CONSTANT, 0);
    return padded;
}

cv::Mat readImage(const std::string& imgPath) {
    // Read the image
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);

    if (image.empty()) {
        std::cerr << "Error: Could not open the image file." << std::endl;
        return cv::Mat();  // Return an empty Mat if the image could not be opened
    }

    return image;
}

int getHSV(const cv::Mat srcImage) {

    if (srcImage.empty()) {
        std::cerr << "Error: Could not open the image file." << std::endl;
        return -1;
    }

    // Get image dimensions
    int rows = srcImage.rows;
    int cols = srcImage.cols;

    // Calculate the coordinates of the pixel in the middle of the image
    int centerX = cols / 2;
    int centerY = rows / 2;

    // Convert BGR to HSV
    cv::Mat3b bgrImage(srcImage);
    cv::Mat3b hsvImage;
    cv::cvtColor(bgrImage, hsvImage, cv::COLOR_BGR2HSV);

    // Get the HSV values of the pixel in the middle of the image
    cv::Vec3b hsvColor = hsvImage(centerY, centerX);

    int hsvValue = (hsvColor[0] << 16) | (hsvColor[1] << 8) | hsvColor[2];

    return hsvValue;
}

int getAverageHSV(const cv::Mat& image, int x, int y) {
    int rows = image.rows;
    int cols = image.cols;
    int scale = 2;

    if (x < scale) {
        x = scale;
    }
    if (y < scale) {
        y = scale;
    }
    if (x > rows - scale) {
        x = rows - scale;
    }
    if (y > cols - scale) {
        y = cols - scale;
    }
    //Should maybe make it dependant on the size of the image
    // Calculate the region of interest (ROI) in the middle of the image
    int startX = x - scale;
    int endX = x + scale;
    int startY = y - scale;
    int endY = y + scale;

    // Initialize accumulators for HSV values
    double totalH = 0, totalS = 0, totalV = 0;
    int numPixels = 0;

    // Iterate over the ROI to accumulate HSV values
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            cv::Vec3b bgrPixel = image.at<cv::Vec3b>(y, x);
            cv::Mat bgrMat(1, 1, CV_8UC3);
            bgrMat.at<cv::Vec3b>(0, 0) = bgrPixel;

            cv::Mat hsvMat;
            cv::cvtColor(bgrMat, hsvMat, cv::COLOR_BGR2HSV);

            cv::Vec3b hsvPixel = hsvMat.at<cv::Vec3b>(0, 0);

            totalH += hsvPixel[0];
            totalS += hsvPixel[1];
            totalV += hsvPixel[2];
            numPixels++;
        }
    }

    // Calculate average HSV values
    int avgH = static_cast<int>(totalH / numPixels);
    int avgS = static_cast<int>(totalS / numPixels);
    int avgV = static_cast<int>(totalV / numPixels);

    // Combine average HSV values into a single integer
    int hsvValue = (avgH << 16) | (avgS << 8) | avgV;

    return hsvValue;
}

cv::Mat getEdges(cv::Mat image) {
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    cv::Mat blurredImage;
    cv::GaussianBlur(gray, blurredImage, cv::Size(1, 1), 0, 0);

    // Apply Canny edge detection
    cv::Mat edges;
    cv::Canny(blurredImage, edges, 50, 135);

    cv::Mat dilatedEdges;
    cv::dilate(edges, dilatedEdges, cv::Mat(), cv::Point(-1, -1), 8);
    //cv::dilate(edges, dilatedEdges, cv::Mat(), cv::Point(-1, -1), 2 + ((image.rows + image.cols) / 1500));

    return dilatedEdges;
}

std::vector<std::vector<cv::Point>> getContours(cv::Mat& image, int invert, int retr) {
    //cv::Mat filteredImage;
    //cv::bilateralFilter(image, filteredImage, 9, 75, 75);  // Adjust parameters as needed
    //image = filteredImage;

    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // Threshold the grayscale image to create a binary mask
    cv::Mat blurredImage;
    cv::GaussianBlur(gray, blurredImage, cv::Size(1, 1), 0, 0);

    // Apply Canny edge detection
    cv::Mat edges;
    cv::Canny(blurredImage, edges, 50, 135);

    // Apply dilation to enhance edges
    cv::Mat dilatedEdges;
    cv::dilate(edges, dilatedEdges, cv::Mat(), cv::Point(-1, -1), 2);

    // Find contours in the mask
    std::vector<std::vector<cv::Point>> contours;
    if (invert == 0) {
        cv::findContours(dilatedEdges, contours, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);
    }
    else {
        cv::findContours(dilatedEdges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    }

    std::vector<std::vector<cv::Point>> filteredContours;
    int minArea = 1000;

    for (const auto& contour : contours) {
        double area = contourArea(contour);
        if (area >= minArea) {
            filteredContours.push_back(contour);
        }
    }

    return filteredContours;
}

cv::Mat findObject(cv::Mat image, int x, int y) {

    // Create a point for the specific pixel
    cv::Point point(x, y);

    int hsvValue = getAverageHSV(image, x, y);
    int h = (hsvValue >> 16) & 0xFF;
    int s = (hsvValue >> 8) & 0xFF;
    int v = hsvValue & 0xFF;

    std::vector<std::vector<cv::Point>> contours;
    if (s < 60) {
        contours = getContours(image, 0, 1);
    } else {
        contours = getContours(image, 1, 1);
    }

    // Check if the specific pixel is within any contour
    for (const auto& contour : contours) {
        if (cv::pointPolygonTest(contour, point, false) >= 0) {
            
            //cv::drawContours(image, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(0, 255, 0), contourThickness(image));

            //Add contour to the list
            addContour(contour);
            cv::circle(image, point, 5, cv::Scalar(255, 0, 0), -1); // Draw the specific pixel
            break;
        }
    }

    // Draw the contours containing the specific pixels
    cv::drawContours(image, contoursList, -1, cv::Scalar(0, 255, 0), contourThickness(image));

    return image;
}

void clearContourList() {
    contoursList.clear();
}

// Function to remove the newest contour from the list (if not empty)
void removeNewestContour() {
    if (!contoursList.empty()) {
        contoursList.pop_back(); // Remove the most recently added contour
    }
}


int findArea() {
    double area = 0;

    //Calculate total area of all contours
    for (const auto& contour : contoursList) {
        area += cv::contourArea(contour);
    }

    //Clear the global contour list
    contoursList.clear();

    return area;
}

int findObjectArea(cv::Mat image, int x, int y) {
    // Create a point for the specific pixel
    cv::Point point(x, y);

    int hsvValue = getAverageHSV(image, x, y);
    int h = (hsvValue >> 16) & 0xFF;
    int s = (hsvValue >> 8) & 0xFF;
    int v = hsvValue & 0xFF;

    std::vector<std::vector<cv::Point>> contours;
    if (s < 50) {
        contours = getContours(image, 0, 1);
    }
    else {
        contours = getContours(image, 1, 1);
    }
    double area = 0;

    // Check if the specific pixel is within any contour
    for (const auto& contour : contours) {
        if (cv::pointPolygonTest(contour, point, false) >= 0) {
            //Find the area of the object
            area = cv::contourArea(contour);

            break;
        }
    }

    return area;
}

cv::Mat identifyAllObjects(cv::Mat& image, int invert) {
    
    std::vector<std::vector<cv::Point>> contours = getContours(image, invert, 1);

    // Draw contours on the original image
    cv::drawContours(image, contours, -1, cv::Scalar(0, 255, 0), contourThickness(image));

    return image;
}

int identifyAllObjectAreas(cv::Mat& image, int invert) {

    std::vector<std::vector<cv::Point>> contours = getContours(image, invert, 1);

    double area = 0;

    for (const auto& contour : contours) {
        area += cv::contourArea(contour);
    }

    return area;
}

int identifyCenterObjectArea(cv::Mat image) {
    int rows = image.rows / 2;
    int cols = image.cols / 2;

    int hsvValue = getAverageHSV(image, cols, rows);
    int h = (hsvValue >> 16) & 0xFF;
    int s = (hsvValue >> 8) & 0xFF;
    int v = hsvValue & 0xFF;

    std::vector<std::vector<cv::Point>> contours;
    if (s < 60) {
        contours = getContours(image, 0, 1);
    }
    else {
        contours = getContours(image, 1, 1);
    }

    // Calculate centroids of contours
    std::vector<cv::Moments> mu(contours.size());
    for (size_t i = 0; i < contours.size(); i++) {
        mu[i] = cv::moments(contours[i]);
    }

    // Find the contour corresponding to the object in the center
    cv::Point2f imageCenter(static_cast<float>(image.cols / 2), static_cast<float>(image.rows / 2));
    int centerContourIndex = -1;
    float minDist = std::numeric_limits<float>::max();

    double area = 0;

    for (size_t i = 0; i < contours.size(); i++) {
        cv::Point2f centroid(static_cast<float>(mu[i].m10 / mu[i].m00), static_cast<float>(mu[i].m01 / mu[i].m00));
        float dist = cv::norm(imageCenter - centroid);

        if (dist < minDist) {
            minDist = dist;
            area = cv::contourArea(contours[i]);
        }
    }

    return area;
}

std::string findCenterOfObject(cv::Mat image) {
    int rows = image.rows / 2;
    int cols = image.cols / 2;

    int hsvValue = getAverageHSV(image, cols, rows);
    int h = (hsvValue >> 16) & 0xFF;
    int s = (hsvValue >> 8) & 0xFF;
    int v = hsvValue & 0xFF;

    std::vector<std::vector<cv::Point>> contours;
    if (s < 60) {
        contours = getContours(image, 0, 1);
    }
    else {
        contours = getContours(image, 1, 1);
    }

    // Calculate centroids of contours
    std::vector<cv::Moments> mu(contours.size());
    for (size_t i = 0; i < contours.size(); i++) {
        mu[i] = cv::moments(contours[i]);
    }

    // Find the contour corresponding to the object closest to the center
    cv::Point2f imageCenter(static_cast<float>(image.cols / 2), static_cast<float>(image.rows / 2));
    int centerContourIndex = -1;
    float minDist = std::numeric_limits<float>::max();

    for (size_t i = 0; i < contours.size(); i++) {
        cv::Point2f centroid(static_cast<float>(mu[i].m10 / mu[i].m00), static_cast<float>(mu[i].m01 / mu[i].m00));
        float dist = cv::norm(imageCenter - centroid);

        if (dist < minDist) {
            minDist = dist;
            centerContourIndex = static_cast<int>(i);
        }
    }

    // Return the centroid of the closest object
    cv::Point2f center;
    if (centerContourIndex != -1) {
        center = cv::Point2f(mu[centerContourIndex].m10 / mu[centerContourIndex].m00, mu[centerContourIndex].m01 / mu[centerContourIndex].m00);
    }
    else {
        center = cv::Point2f(-1, -1); // Return invalid point if no object found
    }

    std::ostringstream oss;
    oss << center.x << ", " << center.y;
    std::string ret = oss.str();

    return ret;
}

cv::Mat identifyCenterObject(cv::Mat image) {
    int rows = image.rows / 2;
    int cols = image.cols / 2;

    int hsvValue = getAverageHSV(image, cols, rows);
    int h = (hsvValue >> 16) & 0xFF;
    int s = (hsvValue >> 8) & 0xFF;
    int v = hsvValue & 0xFF;

    std::vector<std::vector<cv::Point>> contours;
    if (s < 60) {
        contours = getContours(image, 0, 1);
    }
    else {
        contours = getContours(image, 1, 1);
    }

    // Calculate centroids of contours
    std::vector<cv::Moments> mu(contours.size());
    for (size_t i = 0; i < contours.size(); i++) {
        mu[i] = cv::moments(contours[i]);
    }

    // Find the contour corresponding to the object in the center
    cv::Point2f imageCenter(static_cast<float>(image.cols / 2), static_cast<float>(image.rows / 2));
    int centerContourIndex = -1;
    float minDist = std::numeric_limits<float>::max();

    for (size_t i = 0; i < contours.size(); i++) {
        cv::Point2f centroid(static_cast<float>(mu[i].m10 / mu[i].m00), static_cast<float>(mu[i].m01 / mu[i].m00));
        float dist = cv::norm(imageCenter - centroid);

        if (dist < minDist) {
            minDist = dist;
            centerContourIndex = static_cast<int>(i);
        }
    }

    // Draw the contour of the center object onto the image
    cv::drawContours(image, contours, centerContourIndex, cv::Scalar(0, 255, 0), contourThickness(image));

    return image;
}

int main() {
    std::string imgPath = "C:/Users/Sebastian WL/Desktop/Images/blood.jpg";

    cv::Mat image;

    image = readImage(imgPath);
    
    //use box outlines to show objects
    if (false){
        identifyAllObjects(image, 1);
        double area = identifyAllObjectAreas(image, 1);
        std::cout << "Pixels in the objects: " << area << std::endl;
        cv::imshow("Image", image);
        cv::imwrite("C:/Users/Sebastian WL/Desktop/Results/img.jpg", image);
    } else if (true){
        image = findObject(image, 170, 130);
        image = findObject(image, 230, 140);
        cv::imshow("Image", image);
        int area = findArea(image);
        std::cout << "Area of object: " << area << std::endl;
        cv::imwrite("C:/Users/Sebastian WL/Desktop/Results/img.jpg", image);
    } else if (false) {
        image = identifyCenterObject(image);
        std::string middle = findCenterOfObject(image);
        std::cout << "Middle point of object: " << middle << std::endl;
        cv::imshow("Image", image);
    } else if (true) {
        image = getEdges(image);
        cv::imshow("Image", image);
        cv::imwrite("C:/Users/Sebastian WL/Desktop/Results/img.jpg", image);
    }
    waitKey(0);

    return 0;
}
