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


// ---------------------------------- //
// --------- PRE-PROCESSING --------- //
// ---------------------------------- //

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

// ---------------------------------- //
// -------- ^PRE-PROCESSING^ -------- //
// ---------------------------------- //




// ---------------------------------- //
// ----------- WATERSHED ------------ //
// ---------------------------------- //

struct WatershedOutput {
    cv::Mat watershedOutImg;
    int count;
    cv::Mat markers;
};

// ---------- OpenCV ------------- //

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

// ---------- Custom ---------- //

WatershedOutput runCustomWatershed(const cv::Mat& originalImg) 
{
    using namespace cv;
    
    Mat grayImg;
    cvtColor(originalImg, grayImg, COLOR_RGB2GRAY);


    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::Mat opening;
    morphologyEx(grayImg, opening, MORPH_OPEN, kernel, Point(-1, -1), 2);


    int dilationNum = 0;
    Mat sureBg;
    dilate(opening, sureBg, kernel, Point(-1,-1), dilationNum);


    Mat distTransform;
    int distType = DIST_L2;
    int maskSize = 5;
    distanceTransform(opening, distTransform, distType, maskSize);
    
    Mat sureFg;
    double maxDistance = 0.0;
    minMaxLoc(distTransform, nullptr, &maxDistance);

    double thresholdFraction = 0.1;
    double thresholdValue = thresholdFraction * maxDistance;
    double maxBinaryValue = 255.0;
    threshold(distTransform, sureFg, thresholdValue, maxBinaryValue, THRESH_BINARY);

    sureFg.convertTo(sureFg, CV_8U);


    Mat unknown;
    subtract(sureBg, sureFg, unknown);


    Mat markers;
    connectedComponents(sureFg, markers);
    markers += 1; // make background 1 instead of 0
    markers.setTo(0, unknown);



    // Initialize visited map and BFS queue
    Mat visited = Mat::zeros(markers.size(), CV_8U);
    std::queue<Point> bfsQueue;

    const int rows = markers.rows;
    const int cols = markers.cols;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            int markerValue = markers.at<int>(y, x);
            bool isLabeledRegion = markerValue > 1;

            if (isLabeledRegion) {
                bfsQueue.push(Point(x, y));
                visited.at<uchar>(y, x) = 1;
            }
        }
    }

    const std::vector<Point> directions = {
        Point(0, 1),  // right
        Point(1, 0),  // down
        Point(0, -1), // left
        Point(-1, 0)  // up
    };

    while (!bfsQueue.empty()) {
        Point current = bfsQueue.front();
        bfsQueue.pop();

        int currentLabel = markers.at<int>(current);

        for (const auto& dir : directions) {
            Point neighbor = current + dir;

            // Skip out-of-bounds points
            if (neighbor.x < 0 || neighbor.x >= cols || neighbor.y < 0 || neighbor.y >= rows)
                continue;

            uchar& visitedFlag = visited.at<uchar>(neighbor);
            int& neighborLabel = markers.at<int>(neighbor);

            if (!visitedFlag) {
                if (neighborLabel == 0) {
                    // Expand into unlabeled (unknown) region
                    neighborLabel = currentLabel;
                    visitedFlag = 1;
                    bfsQueue.push(neighbor);
                } else if (neighborLabel != currentLabel && neighborLabel != 1) {
                    // Conflict: adjacent to a different region (not background)
                    markers.at<int>(current) = -1; // mark as boundary
                }
            }
        }
    }


    //splitLargeRegions(markers);


    Mat output(markers.size(), CV_8UC3, Scalar(0, 0, 0));

    std::map<int, Vec3b> labelToColor;
    int regionCount = 0;

    for (int y = 0; y < markers.rows; ++y) {
        for (int x = 0; x < markers.cols; ++x) {
            int label = markers.at<int>(y, x);
            Vec3b& pixel = output.at<Vec3b>(y, x);

            if (label == -1) {
                // Boundary between regions
                pixel = Vec3b(255, 255, 255);
            } else if (label > 1) {
                // Assign a random color if this label hasn't been seen yet
                if (labelToColor.find(label) == labelToColor.end()) {
                    Vec3b randomColor(rand() % 256, rand() % 256, rand() % 256);
                    labelToColor[label] = randomColor;
                    regionCount++;
                }
                pixel = labelToColor[label];
            }
        }
    }


    cvtColor(output, output, COLOR_BGR2RGB);

    return { output, regionCount, markers };
}

// ---------------------------------- //
// ---------- ^WATERSHED^ ----------- //
// ---------------------------------- //




// ---------------------------------- //
// -------- other ANALYSIS ---------- //
// ---------------------------------- //

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

cv::Mat drawNSILabels(const cv::Mat& markers) {
    using namespace cv;

    CV_Assert(markers.type() == CV_32S);

    std::map<int, int> labelToIndex;
    std::map<int, Vec3b> labelToColor;
    int index = 0;

    // Prepare base image (color-coded markers)
    Mat output(markers.size(), CV_8UC3, Scalar(0, 0, 0));

    for (int r = 0; r < markers.rows; ++r) {
        for (int c = 0; c < markers.cols; ++c) {
            int label = markers.at<int>(r, c);

            if (label == -1) {
                output.at<Vec3b>(r, c) = Vec3b(255, 255, 255);  // boundary = white
            } else if (label > 1) {
                if (labelToColor.find(label) == labelToColor.end()) {
                    labelToColor[label] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
                    labelToIndex[label] = index++;
                }
                output.at<Vec3b>(r, c) = labelToColor[label];
            }
        }
    }

    // Draw index labels
    for (const auto& [label, idx] : labelToIndex) {
        Mat mask = (markers == label);
        Moments m = moments(mask, true);
        if (m.m00 == 0) continue;

        int cx = static_cast<int>(m.m10 / m.m00);
        int cy = static_cast<int>(m.m01 / m.m00);

        std::string labelStr = std::to_string(idx);
        double fontScale = 0.33;
        int thickness = 1;
        int baseline = 0;

        cv::Size textSize = getTextSize(labelStr, FONT_HERSHEY_SIMPLEX, fontScale, thickness, &baseline);
        Point textOrg(cx - textSize.width / 2, cy + textSize.height / 2);

        putText(output,
                labelStr,
                textOrg,
                FONT_HERSHEY_SIMPLEX,
                fontScale,
                Scalar(255, 255, 255),
                thickness,
                LINE_AA);
    }

    return output;
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

// ---------------------------------- //
// ------- ^other ANALYSIS^ --------- //
// ---------------------------------- //

