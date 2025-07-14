#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <map>
#include <stack>
#include <string>
#include <iostream>
#include <windows.h>
#include <commdlg.h>

#include "tinyfiledialogs.h"
#include "functiondec.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


// --------------------------- //
// ----- Global Variables ---- //
// --------------------------- //

static bool showImageViewer = false;
std::string imageFilename;
cv::Mat originalImage, currentImage, previousImage, nextImage;

// Ctrl+Z/+Shift+Z
std::stack<cv::Mat> undoStack;
std::stack<cv::Mat> redoStack;

// --------------------------- //
// ---- ^Global Variables^ --- //
// --------------------------- //


#include "openglhelper.h"

int main()
{


    // ---------------------------- //
    // - initial GLFW/GLAD/OpenGL - //
    // ---------------------------- //

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // OpenGL context version (e.g. 3.3)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "CytoCaricature", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();

    // Load Segoe UI at 18pt
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 21.0f);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initial values
    GLuint imageTexture = 0;
    int imageWidth = 0;
    int imageHeight = 0;

    // ---------------------------- //
    //  ^initial GLFW/GLAD/OpenGL^  //
    // ---------------------------- //



    
    // --------------------------- //
    // --- Pre-Loop Variables ---- //
    // --------------------------- //

    // Bools for GetKey
    bool openRequested = false;

    // Popup bools
    bool showPrereqPopup = false;
    bool showObjectCntPopup = false;
    bool showNSIEmptyPopup = false;
    bool showNSISummaryPopup = false;
    bool showLicensePopup = false;

    // Other bools
    bool showDataTable = false;
    bool showNSITable = false;

    // Image analysis
    std::vector<double> nsis;
    WatershedOutput watershedOut;
    double avgNSI = 0.0;
    int objectCount = 0;
    
    // Edit bools
    bool singleChannel = false;
    bool isGrayscale = false;
    bool isBinary = false;
    bool cellsSegmented = false;

    // --------------------------- //
    // -- ^Pre-Loop Variables^ --- //
    // --------------------------- //




    // --------------------------- //
    // ------- MAIN LOOP --------- //
    // --------------------------- //


    while (!glfwWindowShouldClose(window)) {

        // Poll and handle events
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --------------------------------//
        // ----------- KEYBINDS -----------//
        // --------------------------------//

        // ============ Ctrl+O =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            openRequested = true;
        }
        if (openRequested) {
            openRequested = false;
            OpenImage(imageTexture, imageWidth, imageHeight);
        }
        // ============ Ctrl+S =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            HWND hwnd = glfwGetWin32Window(window); 
            std::string path = ShowSaveFileDialog(hwnd);

            if (!path.empty() && !currentImage.empty()) {
                cv::Mat currentImageSave = currentImage.clone();
                cv::cvtColor(currentImageSave, currentImageSave, cv::COLOR_RGB2BGR);
                bool success = cv::imwrite(path, currentImageSave);
                if (!success) {
                    std::cerr << "Failed to save image to " << path << "\n";
                }
            }
        }
        // ============ Ctrl+Z =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS &&
            !(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {

            if (!undoStack.empty()) {
                redoStack.push(currentImage.clone());
                currentImage = undoStack.top();
                undoStack.pop();
                cv::cvtColor(currentImage, currentImage, cv::COLOR_BGR2RGB);
                UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            }
        }
        // ========= Ctrl+Shift+Z ======== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {

            if (!redoStack.empty()) {
                undoStack.push(currentImage.clone());
                currentImage = redoStack.top();
                redoStack.pop();
                //cv::cvtColor(currentImage, currentImage, cv::COLOR_BGR2RGB);
                UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            }
        }
        // ============ Ctrl+C =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
            cv::Mat tempBGRimg = originalImage.clone();
            undoStack.push(currentImage.clone());
            while (!redoStack.empty()) redoStack.pop(); // Clear redo history
            currentImage = showBlueChannelOnly(tempBGRimg);
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            singleChannel = true;
        }
        // ============ Ctrl+G =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            //previousImage = currentImage.clone();
            undoStack.push(currentImage.clone());
            while (!redoStack.empty()) redoStack.pop();
            currentImage = toGrayscale(currentImage.clone());
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            isGrayscale = true;
        }
        // ============ Ctrl+B =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
            //previousImage = currentImage.clone();
            undoStack.push(currentImage.clone());
            while (!redoStack.empty()) redoStack.pop();
            currentImage = gaussianFilter(currentImage.clone());
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            //isBlurred = true;
        }
        // ============ Ctrl+P =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            //previousImage = currentImage.clone();
            undoStack.push(currentImage.clone());
            while (!redoStack.empty()) redoStack.pop();
            currentImage = intensityThreshold(currentImage.clone());
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            isBinary = true;
        }
        // ============ Ctrl+W =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            if (singleChannel && isGrayscale && isBinary) {
                watershedOut = runWatershed(currentImage); 
                //previousImage = currentImage.clone();
                undoStack.push(currentImage.clone());
                while (!redoStack.empty()) redoStack.pop();
                currentImage = watershedOut.watershedOutImg;
                objectCount = watershedOut.count;
                //imshow("Display window", currentImage);
                UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                //std::cout << objectCount << std::endl;
                cellsSegmented = true;
                showObjectCntPopup = true;
            } 
            else {
                showPrereqPopup = true;
            }
        }
        // ============ Ctrl+1 =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            //previousImage = currentImage.clone();
            undoStack.push(currentImage.clone());
            while (!redoStack.empty()) redoStack.pop();

            currentImage = showBlueChannelOnly(currentImage);
            currentImage = toGrayscale(currentImage);
            currentImage = gaussianFilter(currentImage);
            currentImage = intensityThreshold(currentImage);
            
            watershedOut = runWatershed(currentImage); 
            currentImage = watershedOut.watershedOutImg;
            objectCount = watershedOut.count;
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            //std::cout << objectCount << std::endl;
            cellsSegmented = true;
            showObjectCntPopup = true;
        }
         // ======== Ctrl+Shift+W ========= //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            if (singleChannel && isGrayscale && isBinary) {
                watershedOut = runCustomWatershed(currentImage); 
                undoStack.push(currentImage.clone());
                while (!redoStack.empty()) redoStack.pop();
                currentImage = watershedOut.watershedOutImg;
                objectCount = watershedOut.count;
                UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                cellsSegmented = true;
                showObjectCntPopup = true;
            } 
            else {
                showPrereqPopup = true;
            }
        }
        // ============ Ctrl+2 =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            undoStack.push(currentImage.clone());
            while (!redoStack.empty()) redoStack.pop();
            
            currentImage = showBlueChannelOnly(currentImage);
            currentImage = toGrayscale(currentImage);
            currentImage = gaussianFilter(currentImage);
            currentImage = intensityThreshold(currentImage);
            
            watershedOut = runCustomWatershed(currentImage); 
            currentImage = watershedOut.watershedOutImg;
            objectCount = watershedOut.count;
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            cellsSegmented = true;
            showObjectCntPopup = true;
        }
        // ============ Ctrl+N =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
            nsis = calculateNSI(watershedOut.markers);
            cv::Mat labeledImgNSI = drawNSILabels(watershedOut.markers);
            undoStack.push(currentImage.clone());
            while (!redoStack.empty()) redoStack.pop();
            currentImage = labeledImgNSI.clone();
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);

            if (nsis.empty()) {
                showNSIEmptyPopup = true;
            } else {
                double sum = 0.0;
                for (double nsi : nsis) {
                    sum += nsi;
                }
                avgNSI = sum / nsis.size();

                showNSISummaryPopup = true;
                showDataTable = true;
                showNSITable = true;
            }
        }
        // ============ Ctrl+H =========== //
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
            cv::Mat NSIheatmap = createNSIHeatmap(watershedOut.markers, nsis);
            //previousImage = currentImage.clone();
            undoStack.push(currentImage.clone());
            while (!redoStack.empty()) redoStack.pop();
            currentImage = NSIheatmap.clone();
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
        }

        // --------------------------------//
        // ---------- ^KEYBINDS^ ----------//
        // --------------------------------//




        // ---------------------------------- //
        // ----------- MENU ITEMS ----------- //
        // ---------------------------------- //

        if (ImGui::BeginMainMenuBar()) {

            if (ImGui::BeginMenu("File")) {

                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                    OpenImage(imageTexture, imageWidth, imageHeight);
                }
                ImGui::MenuItem("Open Directory", "TODO");

                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                        HWND hwnd = glfwGetWin32Window(window); 
                        std::string path = ShowSaveFileDialog(hwnd);

                        if (!path.empty() && !currentImage.empty()) {
                            cv::Mat currentImageSave = currentImage.clone();
                            cv::cvtColor(currentImageSave, currentImageSave, cv::COLOR_RGB2BGR);
                            bool success = cv::imwrite(path, currentImageSave);
                            if (!success) {
                                std::cerr << "Failed to save image to " << path << "\n";
                            }
                        }
                }

                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {

                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                    if (!undoStack.empty()) {
                        redoStack.push(currentImage.clone());
                        currentImage = undoStack.top();
                        undoStack.pop();
                        cv::cvtColor(currentImage, currentImage, cv::COLOR_BGR2RGB);
                        UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    }
                }

                if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z")) {
                    if (!redoStack.empty()) {
                        undoStack.push(currentImage.clone());
                        currentImage = redoStack.top();
                        redoStack.pop();
                        //cv::cvtColor(currentImage, currentImage, cv::COLOR_BGR2RGB);
                        UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Image")) {

                if (ImGui::MenuItem("Isolate Channel", "Ctrl+C")) {
                    undoStack.push(currentImage.clone());
                    while (!redoStack.empty()) redoStack.pop();
                    currentImage = showBlueChannelOnly(originalImage.clone());
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    singleChannel = true;
                }

                if (ImGui::MenuItem("Grayscale", "Ctrl+G")) {
                    undoStack.push(currentImage.clone());
                    while (!redoStack.empty()) redoStack.pop();
                    currentImage = toGrayscale(currentImage.clone());
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    isGrayscale = true;
                }

                if (ImGui::MenuItem("Gaussian Blur", "Ctrl+B")) {
                    undoStack.push(currentImage.clone());
                    while (!redoStack.empty()) redoStack.pop();
                    currentImage = gaussianFilter(currentImage.clone());
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    //isBlurred = true;
                }

                if (ImGui::MenuItem("Threshold Pixel Intensity", "Ctrl+P")) {
                    undoStack.push(currentImage.clone());
                    while (!redoStack.empty()) redoStack.pop();
                    currentImage = intensityThreshold(currentImage.clone());
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    isBinary = true;
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Analyze")) {

                if (ImGui::MenuItem("Object Count (Watershed[OpenCV])", "Ctrl+W")) {
                    if (singleChannel && isGrayscale && isBinary) {
                        watershedOut = runWatershed(currentImage); 
                        undoStack.push(currentImage.clone());
                        while (!redoStack.empty()) redoStack.pop();
                        currentImage = watershedOut.watershedOutImg;
                        objectCount = watershedOut.count;
                        UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                        cellsSegmented = true;
                        showObjectCntPopup = true;
                    } 
                    else {
                        showPrereqPopup = true;
                    }
                }

                if (ImGui::MenuItem("Object Count (pre-processing & Watershed[OpenCV])", "Ctrl+1")) {
                    undoStack.push(currentImage.clone());
                    while (!redoStack.empty()) redoStack.pop();
                    
                    currentImage = showBlueChannelOnly(currentImage);
                    currentImage = toGrayscale(currentImage);
                    currentImage = gaussianFilter(currentImage);
                    currentImage = intensityThreshold(currentImage);
                    
                    WatershedOutput watershedOut = runWatershed(currentImage); 
                    currentImage = watershedOut.watershedOutImg;
                    objectCount = watershedOut.count;
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    cellsSegmented = true;
                    showObjectCntPopup = true;
                }

                if (ImGui::MenuItem("Object Count (Watershed[Custom])", "Ctrl+Shift+W")) {
                    if (singleChannel && isGrayscale && isBinary) {
                        watershedOut = runCustomWatershed(currentImage); 
                        undoStack.push(currentImage.clone());
                        while (!redoStack.empty()) redoStack.pop();
                        currentImage = watershedOut.watershedOutImg;
                        objectCount = watershedOut.count;
                        UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                        cellsSegmented = true;
                        showObjectCntPopup = true;
                    } 
                    else {
                        showPrereqPopup = true;
                    }
                }

                if (ImGui::MenuItem("Object Count (pre-processing & Watershed[Custom])", "Ctrl+2")) {
                    undoStack.push(currentImage.clone());
                    while (!redoStack.empty()) redoStack.pop();
                    
                    currentImage = showBlueChannelOnly(currentImage);
                    currentImage = toGrayscale(currentImage);
                    currentImage = gaussianFilter(currentImage);
                    currentImage = intensityThreshold(currentImage);
                    
                    WatershedOutput watershedOut = runCustomWatershed(currentImage); 
                    currentImage = watershedOut.watershedOutImg;
                    objectCount = watershedOut.count;
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    cellsSegmented = true;
                    showObjectCntPopup = true;
                }

                if (ImGui::MenuItem("NSI Summary", "Ctrl+N")) {
                    nsis = calculateNSI(watershedOut.markers);

                    if (nsis.empty()) {
                        showNSIEmptyPopup = true;
                    } else {
                        double sum = 0.0;
                        for (double nsi : nsis) {
                            sum += nsi;
                        }
                        avgNSI = sum / nsis.size();

                        showNSISummaryPopup = true;
                    }
                }

                if (ImGui::MenuItem("NSI Heatmap", "Ctrl+H")) {
                    cv::Mat NSIheatmap = createNSIHeatmap(watershedOut.markers, nsis);
                    undoStack.push(currentImage.clone());
                    while (!redoStack.empty()) redoStack.pop();
                    currentImage = NSIheatmap.clone();
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("README", "TODO");
                if (ImGui::MenuItem("Copyright")) {
                    showLicensePopup = true;
                    ImGui::OpenPopup("GNU License");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        
        }

        // ---------------------------------- //
        // ---------- ^MENU ITEMS^ ---------- //
        // ---------------------------------- //




        // ------------------------- //
        // ---------- Popups ------- //
        // ------------------------- //

        // for Ctrl+W
        if (showPrereqPopup) {
            ImGui::OpenPopup("Preprocessing Required");
            showPrereqPopup = false;
        } 
        if (!showPrereqPopup && showObjectCntPopup) {
            ImGui::OpenPopup("Object Count");
            showObjectCntPopup = false;
        }
        if (ImGui::BeginPopupModal("Preprocessing Required", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Run the pre-requisite processing first.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
                showPrereqPopup = false;
            }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("Object Count", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Object count: %d", objectCount);
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
                showObjectCntPopup = false;
            }
            ImGui::EndPopup();
        }

        // for Ctrl+N
        if (showNSIEmptyPopup) {
            ImGui::OpenPopup("Objects Un-Segmented");
            showNSIEmptyPopup = false;
        } 
        if (!showNSIEmptyPopup && showNSISummaryPopup) {
            ImGui::OpenPopup("NSI Summary");
            showNSISummaryPopup = false;
        }
        if (ImGui::BeginPopupModal("Objects Un-Segmented", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("No nuclei/objects found so NSI cannot be calculated.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
                showNSIEmptyPopup = false;
            }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("NSI Summary", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Average NSI: %.4f", avgNSI);
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
                showObjectCntPopup = false;
            }
            ImGui::EndPopup();
        }

        // for Copyright
        if (showLicensePopup) {
            ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Always);
            ImGui::OpenPopup("GNU License");
            showNSIEmptyPopup = false;
        } 
        if (ImGui::BeginPopupModal("GNU License", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextWrapped(
                "Copyright (C) 2025 Jeffrey Ock\n\n"
                "This program is free software: you can redistribute it and/or modify\n"
                "it under the terms of the GNU General Public License as published by\n"
                "the Free Software Foundation, either version 3 of the License, or\n"
                "(at your option) any later version.\n\n"
                "This program is distributed in the hope that it will be useful,\n"
                "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
                "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
                "GNU General Public License for more details.\n\n"
                "You should have received a copy of the GNU General Public License\n"
                "along with this program.  If not, see https://www.gnu.org/licenses/"
            );

            ImGui::Separator();

            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
                showLicensePopup = false;
            }

            ImGui::EndPopup();
        }
        

        // ------------------------- //
        // -------- ^Popups^ ------- //
        // ------------------------- //




        // ------------------------- //
        // ------ Table View ------- //
        // ------------------------- //

        if (showNSITable) {
            ShowDataTableAndExport(&showDataTable, nsis);
        }

        // ------------------------- //
        // ----- ^Table View^ ------ //
        // ------------------------- //




        // ------------------------- //
        // ------ Rendering -------- //
        // ------------------------- //

        if (showImageViewer) {
            if (ImGui::Begin(imageFilename.c_str(), &showImageViewer, ImGuiWindowFlags_HorizontalScrollbar)) {
                ImGui::Image((void*)(intptr_t)imageTexture, ImVec2((float)imageWidth, (float)imageHeight));
            }
            ImGui::End();

            if (!showImageViewer) {
                singleChannel = false;
                isGrayscale = false;
                isBinary = false;
                cellsSegmented = false;
            }
        }

        //showImageViewer = true;

        // Rendering
        ImGui::Render();
        int display_w, display_h;

        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);


        // ------------------------- //
        // ------ ^Rendering^ ------ //
        // ------------------------- //


    }

    // --------------------------- //
    // ------ ^MAIN LOOP^ -------- //
    // --------------------------- //




    // --------------------------- //
    // --- OpenGL/GLFW Cleanup --- //
    // --------------------------- //

    if (imageTexture) glDeleteTextures(1, &imageTexture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    // --------------------------- //
    // -- ^OpenGL/GLFW Cleanup^ -- //
    // --------------------------- //


    // ========== CHECK OPENGL VERSION (it is 3.3.0) ========== //
    /**
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // ====================== OLD CODE ======================== //

    // Create window and OpenGL context (specify version if you want)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(640, 480, "OpenGL Version Check", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize Glad (load OpenGL function pointers)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // Query OpenGL version
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL Version: " << version << std::endl;

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
    */

    // ========== CLI VERSION (v1.0.0) ==========

    /**
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
    */

    return 0;
}


