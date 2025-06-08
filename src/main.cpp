#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <map>

#include <iostream>
#include "tinyfiledialogs.h"

using namespace cv;

Mat showBlueChannelOnly(const Mat& img)
{
    Mat blueOnly = img.clone();

    std::vector<Mat> channels(3);
    split(blueOnly, channels);

    channels[1] = Mat::zeros(channels[1].size(), channels[1].type()); // Zero Green
    channels[2] = Mat::zeros(channels[2].size(), channels[2].type()); // Zero Red

    merge(channels, blueOnly);
    return blueOnly;
}


Mat toGrayscale(const Mat& img)
{
    Mat grayscale = img.clone();

    cvtColor(img, grayscale, COLOR_BGR2GRAY);

    return grayscale;
}


Mat gaussianFilter(const Mat& img)
{
    Mat blurredImg = img.clone();

    GaussianBlur(img, blurredImg, Size(0, 0), 3.0);

    return blurredImg;
}

Mat intensityThreshold(const Mat& img)
{
    Mat gray, binary;

    if (img.channels() == 3)
        cvtColor(img, gray, COLOR_BGR2GRAY);
    else
        gray = img;

    threshold(gray, binary, 0, 255, THRESH_BINARY | THRESH_OTSU);

    return binary;
}

struct WatershedOutput {
    cv::Mat watershedOutImg;
    int count;
};

//TODO
WatershedOutput runWatershed(const cv::Mat& img) 
{
    using namespace cv;
    // Noise removal with morphological opening
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    Mat opening;
    morphologyEx(img, opening, MORPH_OPEN, kernel, Point(-1, -1), 2);

    // Sure background area (dilated)
    Mat sureBg;
    dilate(opening, sureBg, kernel, Point(-1, -1), 3);

    // Sure foreground area using distance transform
    Mat distTransform;
    distanceTransform(opening, distTransform, DIST_L2, 5);
    Mat sureFg;
    double minVal, maxVal;
    minMaxLoc(distTransform, &minVal, &maxVal);
    threshold(distTransform, sureFg, 0.4 * maxVal, 255, 0);
    sureFg.convertTo(sureFg, CV_8U);

    // Unknown region = background - foreground
    Mat unknown;
    subtract(sureBg, sureFg, unknown);

    // Label markers
    Mat markers;
    connectedComponents(sureFg, markers);
    markers += 1; // make sure background is not 0
    markers.setTo(0, unknown); // mark unknown as 0

    // Prepare input for watershed (needs 3 channels)
    Mat colorImg;
    if (img.channels() == 1)
        cvtColor(img, colorImg, COLOR_GRAY2BGR);
    else
        colorImg = img.clone();

    // Apply watershed
    watershed(colorImg, markers);

    // Generate output image
    Mat output = Mat::zeros(markers.size(), CV_8UC3);
    int count = 0;
    std::map<int, Vec3b> colorMap;

    for (int r = 0; r < markers.rows; ++r) {
        for (int c = 0; c < markers.cols; ++c) {
            int idx = markers.at<int>(r, c);
            if (idx == -1) {
                output.at<Vec3b>(r, c) = Vec3b(255, 255, 255); // boundary
            }
            else if (idx > 1) {
                if (colorMap.find(idx) == colorMap.end()) {
                    colorMap[idx] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
                    count++;
                }
                output.at<Vec3b>(r, c) = colorMap[idx];
            }
        }
    }

    return { output, count };
}

int main()
{
    const char* filetypes[] = { "*.jpg", "*.png", "*.tif", "*.bmp" };
    const char* image_path = tinyfd_openFileDialog(
        "Select an image",
        "",
        4, filetypes,
        "Image files",
        0);

    if (!image_path) {
        std::cout << "No file selected.\n";
        return 1;
    }

    Mat img = imread(image_path, IMREAD_COLOR);

    if (img.empty())
    {
        std::cout << "Could not read the image: " << image_path << std::endl;
        return 1;
    }

    imshow("Display window", img);
    Mat currentImg = img.clone();

    bool singleChannel = false;
    bool isGrayscale = false;
    bool isBinary = false;

    while (true) {
        int key = waitKey(0);

        if (key == 27) break; // 27 -> Esc

        if (key == 'c') {
            currentImg = showBlueChannelOnly(currentImg);
            singleChannel = true;
            imshow("Display window", currentImg);
        }
        else if (key == 'g') {
            currentImg = toGrayscale(currentImg);
            isGrayscale = true;
            imshow("Display window", currentImg);
        }
        else if (key == 'b') {
            currentImg = gaussianFilter(currentImg); 
            imshow("Display window", currentImg);
        }
        else if (key == 'p') {
            currentImg = intensityThreshold(currentImg); 
            isBinary = true;
            imshow("Display window", currentImg);
        }
        else if (key == 'w') {
            if (singleChannel && isGrayscale && isBinary) {
                WatershedOutput watershedOut= runWatershed(currentImg); 
                currentImg = watershedOut.watershedOutImg;
                int objectCount = watershedOut.count;
                imshow("Display window", currentImg);
                std::cout << objectCount << std::endl;
            } 
            else {
                std::cout << "Run the pre-requisite processing first." << std::endl;
            }
            
        }
        else if (key == 13) {
            currentImg = showBlueChannelOnly(currentImg);
            currentImg = toGrayscale(currentImg);
            currentImg = gaussianFilter(currentImg);
            currentImg = intensityThreshold(currentImg);
            
            WatershedOutput watershedOut= runWatershed(currentImg); 
            currentImg = watershedOut.watershedOutImg;
            int objectCount = watershedOut.count;
            imshow("Display window", currentImg);

            std::cout << "Object Count:" << std::endl;
            std::cout << objectCount << std::endl;
        }
        else if (key == 'r') {
            currentImg = img.clone(); 
            singleChannel = false;
            isGrayscale = false;
            isBinary = false;
            imshow("Display window", currentImg);
        }
    }

    return 0;
}


