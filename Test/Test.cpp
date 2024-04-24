#include <opencv2/opencv.hpp>
#include <iostream>
using namespace cv;

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

cv::Mat createMask(const cv::Mat& inputImage, const cv::Scalar& lowerBound, const cv::Scalar& upperBound) {
    // Convert image to HSV color space
    cv::Mat inputImageHSV;
    cv::cvtColor(inputImage, inputImageHSV, cv::COLOR_BGR2HSV);

    // Create mask
    cv::Mat mask;
    cv::inRange(inputImageHSV, lowerBound, upperBound, mask);

    return mask;
}

cv::Mat applyMask(const cv::Mat& image, const cv::Mat& mask) {
    cv::Mat resultImage;
    cv::bitwise_and(image, image, resultImage, mask);

    return resultImage;
}

std::vector<std::vector<cv::Point>> getContours(cv::Mat& image, int invert, int retr) {
    cv::Mat filteredImage;
    cv::bilateralFilter(image, filteredImage, 9, 75, 75);  // Adjust parameters as needed
    image = filteredImage;

    cv::Mat gray;
    cv::cvtColor(filteredImage, gray, cv::COLOR_BGR2GRAY);

    //cv::Mat equalizedImage;
    //cv::equalizeHist(gray, equalizedImage);

    // Threshold the grayscale image to create a binary mask
    
    cv::Mat mask;
    if (invert == 0){
        cv::threshold(gray, mask, 0, 255, cv::THRESH_OTSU);
    } else {
        cv::threshold(gray, mask, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    }

    int padSize = 1; // adjust the padding size as needed
    cv::Mat paddedImage = padImage(mask, padSize);
    
    cv::Mat blurredImage;
    cv::GaussianBlur(paddedImage, blurredImage, cv::Size(5, 5), 1.5);

    // Apply Canny edge detection
    cv::Mat edges;
    cv::Canny(blurredImage, edges, 50, 135);

    //cv::Rect roi(padSize, padSize, image.cols, image.rows);
    //cv::Mat detectedEdges = edges(roi);

    // Apply dilation to enhance edges
    cv::Mat dilatedEdges;
    cv::dilate(edges, dilatedEdges, cv::Mat(), cv::Point(-1, -1), 1);

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
            // Draw the contour containing the specific pixel
            cv::drawContours(image, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(0, 255, 0), contourThickness(image));
            cv::circle(image, point, 5, cv::Scalar(255, 0, 0), -1); // Draw the specific pixel
            break;
        }
    }

    return image;
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

void removeBackground(cv::Mat& image) {
    // Convert the image to grayscale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // Threshold the grayscale image to create a binary mask
    cv::Mat mask;
    cv::threshold(gray, mask, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // Find contours in the mask
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Create a mask for the contours
    cv::Mat contourMask = cv::Mat::zeros(image.size(), CV_8UC1);
    cv::drawContours(contourMask, contours, -1, cv::Scalar(255), cv::FILLED);

    // Apply the mask to the original image to keep only the objects inside the contours
    cv::Mat result;
    image.copyTo(result, contourMask);

    // Update the original image with the result
    image = result;
}

cv::Mat identifyColor(cv::Mat image) {
    int range = 4;
    cv::Mat resultImage;
    cv::Mat mask;

    int hsvValue;

    int rows = image.rows;
    int cols = image.cols;

    if (cols < 8 || rows < 8) {
        hsvValue = getHSV(image);
    }

    hsvValue = getAverageHSV(image, cols / 2, rows / 2);

    if (hsvValue != -1) {
        int h = (hsvValue >> 16) & 0xFF;
        int s = (hsvValue >> 8) & 0xFF;
        int v = hsvValue & 0xFF;

        int minSat, maxSat;
        int minVal, maxVal;

        if (s < 100) {
            minSat = 0;
            maxSat = s + 100;
            if (s < 40) {
                range = 30;
                if (s < 20) {
                    range = 60;
                    if (s < 10) {
                        range = 90;
                    }
                }
            }
        } else if (s > 155) {
            minSat = s - 100;
            maxSat = 255;
        } else {
            minSat = s - 100;
            maxSat = s + 100;
        }

        if (v < 50) {
            minVal = 0;
            maxVal = v + 30;
        } else {
            minVal = 40;
            maxVal = 255;
        }

        cv::Scalar lowerBound;
        cv::Scalar upperBound;

        if (h < range || h >(180 - range)) {

            cv::Scalar lowerBound2;
            cv::Scalar upperBound2;

            cv::Mat mask1, mask2;

            if (h < range) {
                int minHue1 = 0, maxHue1 = h + range;
                int minHue2 = 179 - (range - h), maxHue2 = 180;

                lowerBound = cv::Scalar(minHue1, minSat, minVal);
                upperBound = cv::Scalar(maxHue1, maxSat, maxVal);

                lowerBound2 = cv::Scalar(minHue2, minSat, minVal);
                upperBound2 = cv::Scalar(maxHue2, maxSat, maxVal);
            }
            else {
                int minHue1 = h - range, maxHue1 = 180;
                int minHue2 = 0, maxHue2 = range - (180 - h);

                lowerBound = cv::Scalar(minHue1, minSat, minVal);
                upperBound = cv::Scalar(maxHue1, maxSat, maxVal);

                lowerBound2 = cv::Scalar(minHue2, minSat, minVal);
                upperBound2 = cv::Scalar(maxHue2, maxSat, maxVal);
            }

            mask1 = createMask(image, lowerBound, upperBound);
            mask2 = createMask(image, lowerBound2, upperBound2);

            mask = mask1 | mask2;

            resultImage = applyMask(image, mask);
        }
        else {
            int minHue = h - range, maxHue = h + range;

            lowerBound = cv::Scalar(minHue, minSat, minVal);
            upperBound = cv::Scalar(maxHue, maxSat, maxVal);

            mask = createMask(image, lowerBound, upperBound);

            resultImage = applyMask(image, mask);
        }
    }

    return resultImage;
}

cv::Mat identifyColor(cv::Mat image, int x, int y) {
    int range = 4;
    cv::Mat resultImage;
    cv::Mat mask;

    int hsvValue = getAverageHSV(image, x, y);

    if (hsvValue != -1) {
        int h = (hsvValue >> 16) & 0xFF;
        int s = (hsvValue >> 8) & 0xFF;
        int v = hsvValue & 0xFF;

        int minSat, maxSat;
        int minVal, maxVal;

        if (s < 100) {
            minSat = 0;
            maxSat = s + 100;
            if (s < 40) {
                range = 30;
                if (s < 20) {
                    range = 60;
                    if (s < 10) {
                        range = 90;
                    }
                }
            }
        }
        else if (s > 155) {
            minSat = s - 100;
            maxSat = 255;
        }
        else {
            minSat = s - 100;
            maxSat = s + 100;
        }

        if (v < 50) {
            minVal = 0;
            maxVal = v + 30;
        }
        else {
            minVal = 40;
            maxVal = 255;
        }

        cv::Scalar lowerBound;
        cv::Scalar upperBound;

        if (h < range || h >(180 - range)) {

            cv::Scalar lowerBound2;
            cv::Scalar upperBound2;

            cv::Mat mask1, mask2;

            if (h < range) {
                int minHue1 = 0, maxHue1 = h + range;
                int minHue2 = 179 - (range - h), maxHue2 = 180;

                lowerBound = cv::Scalar(minHue1, minSat, minVal);
                upperBound = cv::Scalar(maxHue1, maxSat, maxVal);

                lowerBound2 = cv::Scalar(minHue2, minSat, minVal);
                upperBound2 = cv::Scalar(maxHue2, maxSat, maxVal);
            }
            else {
                int minHue1 = h - range, maxHue1 = 180;
                int minHue2 = 0, maxHue2 = range - (180 - h);

                lowerBound = cv::Scalar(minHue1, minSat, minVal);
                upperBound = cv::Scalar(maxHue1, maxSat, maxVal);

                lowerBound2 = cv::Scalar(minHue2, minSat, minVal);
                upperBound2 = cv::Scalar(maxHue2, maxSat, maxVal);
            }

            mask1 = createMask(image, lowerBound, upperBound);
            mask2 = createMask(image, lowerBound2, upperBound2);

            mask = mask1 | mask2;

            resultImage = applyMask(image, mask);
        }
        else {
            int minHue = h - range, maxHue = h + range;

            lowerBound = cv::Scalar(minHue, minSat, minVal);
            upperBound = cv::Scalar(maxHue, maxSat, maxVal);

            mask = createMask(image, lowerBound, upperBound);

            resultImage = applyMask(image, mask);
        }
    }

    return resultImage;
}

int main() {
    std::string imgPath = "C:/Users/Sebastian WL/Desktop/Images/can.webp";

    cv::Mat image;

    image = readImage(imgPath);

    //use box outlines to show objects
    if (false) {
        image = identifyColor(image);
        identifyAllObjects(image, 0);
        cv::imshow("Image", image);
    } else if (true){
        identifyAllObjects(image, 1);
        double area = identifyAllObjectAreas(image, 1);
        std::cout << "Pixels in the objects: " << area << std::endl;
        cv::imshow("Image", image);
    } else if (false){
        image = findObject(image, 100, 100);
        int area = findObjectArea(image, 100, 100);
        std::cout << "Area of object: " << area << std::endl;
        cv::imshow("Image", image);
    } else if (false) {
        image = identifyCenterObject(image);
        std::string middle = findCenterOfObject(image);
        std::cout << "Middle point of object: " << middle << std::endl;
        cv::imshow("Image", image);
    }
    waitKey(0);

    return 0;
}
