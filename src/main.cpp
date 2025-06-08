#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

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

//TODO
Mat toGrayscale(const Mat& img)
{
    Mat grayscale = img.clone();

    cvtColor(img, grayscale, COLOR_BGR2GRAY);

    return grayscale;
}

//TODO
Mat gaussianFilter(const Mat& img)
{
    Mat blurredImg = img.clone();

    return blurredImg;
}

//TODO
Mat intensityThreshold(const Mat& img)
{
    Mat pixIntensityThresh = img.clone();

    return pixIntensityThresh;
}

struct WatershedOutput {
    Mat watershedOutImg;
    int count;
};

//TODO
WatershedOutput runWatershed(const Mat& img) 
{
    Mat watershedImg = img.clone();
    int objCount = 0;

    return {watershedImg, objCount};
}

Mat generateLabelMatrix(const Mat& img)
{
    Mat labelMatrixImg = img.clone();

    return labelMatrixImg;
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

    while (true) {
        int key = waitKey(0);

        if (key == 27) break; // 27 -> Esc

        if (key == 'c') {
            currentImg = showBlueChannelOnly(currentImg);
            imshow("Display window", currentImg);
        }
        else if (key == 'g') {
            currentImg = toGrayscale(currentImg);
            imshow("Display window", currentImg);
        }
        else if (key == 'b') {
            currentImg = gaussianFilter(currentImg); 
            imshow("Display window", currentImg);
        }
        else if (key == 'p') {
            currentImg = intensityThreshold(currentImg); 
            imshow("Display window", currentImg);
        }
        else if (key == 'w') {
            WatershedOutput watershedOut= runWatershed(currentImg); 
            currentImg = watershedOut.watershedOutImg;
            int objectCount = watershedOut.count;
            imshow("Display window", currentImg);
            std::cout << objectCount << std::endl;
        }
        else if (key == 'r') {
            currentImg = img.clone(); 
            imshow("Display window", currentImg);
        }
    }

    return 0;
}


