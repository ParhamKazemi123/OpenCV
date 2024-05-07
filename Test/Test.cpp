#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
using namespace cv;

// Global vector to store contours
std::vector<std::vector<cv::Point>> contoursList;

cv::Scalar contourColor = cv::Scalar(222, 181, 255);

int areaInfo = 0;
cv::Mat imageInfo;
cv::Point centerInfo(0, 0);

double getArea() {
    return areaInfo;
}

cv::Mat getImage() {
    return imageInfo;
}

cv::Point getCenter() {
    return centerInfo;
}

void drawWeightedContour(cv::Mat image, std::vector<cv::Point> contour) {
    cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
    std::vector<std::vector<cv::Point>> contours = { contour };
    cv::drawContours(mask, contours, -1, cv::Scalar(255), cv::FILLED);

    // Create an opaque version of the original image
    cv::Mat overlayedImage = image.clone();

    // Apply the mask to the overlayed image
    overlayedImage.setTo(contourColor, mask);

    // Blend the overlayed image with the original image
    double alpha = 0.4;
    cv::addWeighted(overlayedImage, alpha, image, 1.0 - alpha, 0, image);
    cv::drawContours(image, contours, -1, contourColor, 1 + ((image.rows + image.cols) / 400));
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

cv::Mat getEdges(cv::Mat image) {
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    cv::Mat blurredImage;
    cv::GaussianBlur(gray, blurredImage, cv::Size(1, 1), 5, 5);

    // Apply Canny edge detection
    cv::Mat edges;
    cv::Canny(blurredImage, edges, 30, 60);

    cv::Mat dilatedEdges;
    cv::dilate(edges, dilatedEdges, cv::Mat(), cv::Point(-1, -1), 1 + ((image.rows + image.cols) / 1500));

    return dilatedEdges;
}

std::vector<std::vector<cv::Point>> getContours(cv::Mat& image) {

    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    cv::Mat blurredImage;
    cv::GaussianBlur(gray, blurredImage, cv::Size(1, 1), 0, 0);

    // Apply Canny edge detection
    cv::Mat edges;
    cv::Canny(blurredImage, edges, 50, 135);

    // Apply dilation to enhance edges
    cv::Mat dilatedEdges;
    cv::dilate(edges, dilatedEdges, cv::Mat(), cv::Point(-1, -1), 2 + ((image.rows + image.cols) / 1500));

    // Find contours in the mask
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(dilatedEdges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<std::vector<cv::Point>> filteredContours;
    int minArea = 2000;

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

    std::vector<std::vector<cv::Point>> contours = getContours(image);

    //check if the sent point is already in a contour
    for (const auto& contour : contoursList) {
        if (cv::pointPolygonTest(contour, point, false) >= 0) {

            cv::drawContours(image, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(0, 255, 0), 2 + ((image.rows + image.cols) / 200));

            return image;
        }
    }

    // One problem is that smaller contours could still be added to larger ones, which could cause problems
    // Check if the specific pixel is within any contour
    for (const auto& contour : contours) {
        if (cv::pointPolygonTest(contour, point, false) >= 0) {

            //Add contour to the list
            contoursList.push_back(contour);
            cv::circle(image, point, 5, cv::Scalar(255, 0, 0), -1); // Draw the specific pixel
            break;
        }
    }

    // Draw the contours containing the specific pixels
    cv::drawContours(image, contoursList, -1, contourColor, 2 + ((image.rows + image.cols) / 200));

    return image;
}

void clearContourList() {
    contoursList.clear();
}

// Function to remove the newest contour from the list (if not empty)
void removeNewestContour(cv::Mat image) {
    if (!contoursList.empty()) {
        contoursList.pop_back(); // Remove the most recently added contour
    }

    cv::drawContours(image, contoursList, -1, contourColor, 2 + ((image.rows + image.cols) / 200));
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

void findObjectInfo(cv::Mat image, int x, int y) {
    imageInfo = image;

    // Create a point for the specific pixel
    cv::Point point(x, y);

    std::vector<std::vector<cv::Point>> contours = getContours(image);
    double area = 0;
    cv::Point center(0, 0);

    // Check if the specific pixel is within any contour
    for (const auto& contour : contours) {
        if (cv::pointPolygonTest(contour, point, false) >= 0) {

            // Calculate the area
            area = cv::contourArea(contour);

            // Draw the contour containing the specific pixel
            //drawWeightedContour(image, contour);
            cv::drawContours(image, std::vector<std::vector<cv::Point>>{contour}, -1, contourColor, 1 + ((image.rows + image.cols) / 400));
            cv::circle(image, point, 5, cv::Scalar(0, 0, 255), -1); // Draw the specific pixel

            // Calculate center
            center = cv::Point(0, 0);
            for (const auto& p : contour) {
                center += p;
            }
            center.x /= contour.size();
            center.y /= contour.size();

            areaInfo = area;
            imageInfo = image;
            centerInfo = center;

            break;
        }
    }
}

void centerObjectInfo(cv::Mat image) {

    std::vector<std::vector<cv::Point>> contours = getContours(image);
    double area = 0;
    cv::Point center(0, 0);

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
            center.x = mu[i].m10 / mu[i].m00;
            center.y = mu[i].m01 / mu[i].m00;
            area = cv::contourArea(contours[i]);
        }
    }

    // Draw the contour of the center object onto the image
    if (centerContourIndex > -1) {
        //drawWeightedContour(image, contours[centerContourIndex]);
        cv::drawContours(image, contours, centerContourIndex, contourColor, 1 + ((image.rows + image.cols) / 400));
    }

    areaInfo = area;
    imageInfo = image;
    centerInfo = center;
}

int main() {
    std::string imgPath = "C:/Users/Sebastian WL/Desktop/Images/blood.jpg";

    cv::Mat image;

    image = readImage(imgPath);

    auto start = std::chrono::high_resolution_clock::now();
    
    //use box outlines to show objects
    if (false){
        centerObjectInfo(image);
        cv::imshow("Image", image);
    } else if (true){
        image = findObject(image, 170, 130);
        image = findObject(image, 230, 140);
        cv::imshow("Image", image);
        int area = findArea();
        std::cout << "Area of object: " << area << std::endl;
        cv::imwrite("C:/Users/Sebastian WL/Desktop/Results/img.jpg", image);

    } else if (true) {
        //centerObjectInfo(image);
        findObjectInfo(image, image.cols / 2, image.rows / 2);
        //getEdges(image);
        //getContours(image);
        cv::imshow("Image", image);
        cv::imwrite("C:/Users/Sebastian WL/Desktop/Results/img.jpg", image);
    } else if (false) {
        image = getEdges(image);
        cv::imshow("Image", image);
        cv::imwrite("C:/Users/Sebastian WL/Desktop/Results/img.jpg", image);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;

    waitKey(0);

    return 0;
}
