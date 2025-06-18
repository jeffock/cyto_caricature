#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <map>
#include <set>
#include <algorithm>
#include <vector>

#include <iostream>

using namespace cv;

cv::Mat showBlueChannelOnly(const cv::Mat& imgOriginal)
{
    // BGR

    cv::Mat img = imgOriginal.clone();
    //cv::Mat img8bit;

    /**
    // DEBUG & convert 16bit to 8bit
    std::cout << "img.depth(): " << img.depth() << std::endl;
    std::cout << "CV_8U: " << CV_8U << ", CV_16U: " << CV_16U << std::endl;
    std::cout << "Original img type: " << img.type() << std::endl; // Expect CV_8UC3 (type 16)
    if (img.depth() != CV_8U) {
        std::cout << "Convert from 16->8bit" << std::endl;
        img8bit = img / 257; // Normalize 16-bit to 8-bit
    } else {
        img8bit = img.clone();
    }
    std::cout << "Original img type: " << img8bit.type() << std::endl; // Expect CV_8UC3 (type 16)
    */

    //cv::imshow("img", img);

    std::vector<cv::Mat> channels(3);
    cv::split(img, channels);  // B, G, R

    //DEBUG
    //double minVal, maxVal;
    //cv::minMaxLoc(channels[0], &minVal, &maxVal);
    //std::cout << "Blue channel stats: min=" << minVal << ", max=" << maxVal << std::endl;

    channels[1] = cv::Mat::zeros(channels[1].size(), channels[1].type()); // Zero G
    channels[2] = cv::Mat::zeros(channels[2].size(), channels[2].type()); // Zero R

    cv::Mat blueOnly;
    cv::merge(channels, blueOnly);

    //cv::imshow("Non-OpenGL", blueOnly);

    cv::cvtColor(blueOnly, blueOnly, cv::COLOR_BGR2RGB); // Convert for OpenGL

    return blueOnly;
    

    // RGB
    /**
    std::vector<cv::Mat> channels(3);
    cv::split(img, channels);

    // Zero out red and green (channels 0 and 1)
    channels[0] = cv::Mat::zeros(channels[0].size(), channels[0].type()); // Red
    channels[1] = cv::Mat::zeros(channels[1].size(), channels[1].type()); // Green

    cv::Mat blueOnly;
    cv::merge(channels, blueOnly);
    return blueOnly;
    */
}


Mat toGrayscale(const Mat& img)
{
    Mat grayscale = img.clone();
    Mat gray3ch;

    cvtColor(img, grayscale, COLOR_RGB2GRAY);
    cvtColor(grayscale, gray3ch, COLOR_GRAY2RGB);  // Make it 3-channel again
    return gray3ch;

    return gray3ch;
}


Mat gaussianFilter(const Mat& img)
{
    Mat blurredImg = img.clone();

    GaussianBlur(img, blurredImg, Size(0, 0), 3.0);
    cv::cvtColor(blurredImg, blurredImg, cv::COLOR_BGR2RGB); // Convert for OpenGL

    return blurredImg;
}

Mat intensityThreshold(const Mat& img)
{
    Mat gray, binary, binary3ch;

    if (img.channels() == 3)
        cvtColor(img, gray, COLOR_BGR2GRAY);
    else
        gray = img;

    threshold(gray, binary, 0, 255, THRESH_BINARY | THRESH_OTSU);

    cvtColor(binary, binary3ch, COLOR_GRAY2RGB);  // Make it 3-channel again
    return binary3ch;
}

struct WatershedOutput {
    cv::Mat watershedOutImg;
    int count;
    cv::Mat markers;
};

WatershedOutput runWatershed(const cv::Mat& originalImg) 
{
    using namespace cv;

    Mat img = originalImg.clone();
    cvtColor(img, img, COLOR_RGB2GRAY);

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

    cvtColor(output, output, COLOR_BGR2RGB);

    return { output, count, markers };
}

std::vector<double> calculateNSI(const cv::Mat& markersArg) {
    using namespace cv;

    Mat markers = markersArg.clone();

    std::vector<double> nsis;

    //DEBUGGING
    //std::cout << "NSI FUNCTION RUNNING" << std::endl;

    // Skip labels: 0 (unknown) and -1 (watershed boundary)
    std::set<int> labels;
    for (int r = 0; r < markers.rows; ++r) {
        for (int c = 0; c < markers.cols; ++c) {
            int label = markers.at<int>(r, c);
            if (label > 1) {
                labels.insert(label);
            }
        }
    }

    for (int label : labels) {
        // Create binary mask for this label
        Mat mask = (markers == label);

        // Find contours for perimeter
        std::vector<std::vector<Point>> contours;
        findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        double area = countNonZero(mask);
        double perimeter = 0.0;

        if (!contours.empty()) {
            perimeter = arcLength(contours[0], true);
        }

        // Avoid divide-by-zero
        if (area > 0)
            nsis.push_back((perimeter * perimeter) / (4 * CV_PI * area));
        else
            nsis.push_back(0.0);
    }

    return nsis;
}

// Helper: map normalized NSI (0-1) to blue-red color
cv::Vec3b nsiToColor(float normVal) {
    uchar r = static_cast<uchar>(255 * normVal);
    uchar g = 0;
    uchar b = static_cast<uchar>(255 * (1.0f - normVal));
    return cv::Vec3b(b, g, r);
}

// Main function to create NSI heatmap
cv::Mat createNSIHeatmap(const cv::Mat& markers, const std::vector<double>& nsis) {
    cv::Mat heatmap = cv::Mat::zeros(markers.size(), CV_8UC3);

    if (nsis.empty()) return heatmap;

    // Find min and max NSI for normalization
    double minNSI = *std::min_element(nsis.begin(), nsis.end());
    double maxNSI = *std::max_element(nsis.begin(), nsis.end());

    // Print NSI min/max info with colors
    std::cout << "Minimum NSI: " << minNSI << " (blue color: BGR = "
    << (int)(255) << ", " << 0 << ", " << 0 << ")\n";
    std::cout << "Maximum NSI: " << maxNSI << " (red color: BGR = "
    << 0 << ", " << 0 << ", " << (int)(255) << ")\n";

    std::map<int, cv::Vec3b> nsiColors;
    int idx = 0;

    // Assign a color to each label starting from label=2 (as per your markers)
    for (int label = 2; label < 2 + static_cast<int>(nsis.size()); ++label) {
        float normVal = 0.f;
        if (maxNSI != minNSI) {
            normVal = static_cast<float>((nsis[idx] - minNSI) / (maxNSI - minNSI));
        }
        nsiColors[label] = nsiToColor(normVal);
        idx++;
    }

    // Color each pixel according to its segment's NSI color
    for (int r = 0; r < markers.rows; ++r) {
        for (int c = 0; c < markers.cols; ++c) {
            int label = markers.at<int>(r, c);
            if (label > 1) {
                auto it = nsiColors.find(label);
                if (it != nsiColors.end()) {
                    heatmap.at<cv::Vec3b>(r, c) = it->second;
                }
            }
        }
    }

    return heatmap;
}