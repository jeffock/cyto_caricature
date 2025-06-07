#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>
#include "tinyfiledialogs.h"

using namespace cv;

// Function that returns an image with only the blue channel preserved
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

int main()
{
    // Open file dialog to choose image
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
    int firstKey = waitKey(0); // Wait for a keystroke

    if (firstKey == 'c')
    {
        Mat blueOnly = showBlueChannelOnly(img);
        imshow("Display window", blueOnly);
        waitKey(0);
    }

    return 0;
}


