#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <map>

#include <iostream>
#include "tinyfiledialogs.h"
#include "functiondec.h"

// GUI/windowing/rendering
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace cv;

int main()
{

    //IMGUI DEMO IMPL

    //IMGUI DEMO IMPL

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
    bool cellsSegmented = false;

    WatershedOutput watershedOut;
    std::vector<double> nsis;

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
                WatershedOutput watershedOut = runWatershed(currentImg); 
                currentImg = watershedOut.watershedOutImg;
                int objectCount = watershedOut.count;
                imshow("Display window", currentImg);
                std::cout << objectCount << std::endl;
                cellsSegmented = true;
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
            
            watershedOut = runWatershed(currentImg); 
            currentImg = watershedOut.watershedOutImg;
            int objectCount = watershedOut.count;
            imshow("Display window", currentImg);

            cellsSegmented = true;

            std::cout << "Object Count:" << std::endl;
            std::cout << objectCount << std::endl;
        }
        else if (key == 's' && cellsSegmented) {
            //DEBUGGING:
            //std::cout << "ELIF BLOCK RUNNING" << std::endl;

            nsis = calculateNSI(watershedOut.markers);

            if (nsis.empty()) {
                std::cout << "No nuclei found to calculate NSI.\n";
            } else {
                double sum = 0.0;
                for (double nsi : nsis) {
                    sum += nsi;
                }
                double avgNSI = sum / nsis.size();

                std::cout << "Average Nuclear Spreading Index (NSI): " << avgNSI << std::endl;
            }
        }
        else if (key == 'h' && !nsis.empty()) {
            cv::Mat NSIheatmap = createNSIHeatmap(watershedOut.markers, nsis);

            cv::imshow("NSI Heatmap", NSIheatmap);
        }
        else if (key == 'r') {
            currentImg = img.clone(); 
            singleChannel = false;
            isGrayscale = false;
            isBinary = false;
            imshow("Display window", currentImg);

            if (cellsSegmented) {
                cellsSegmented = false;
            }
        }
    }

    return 0;
}


