#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <map>
#include <string>

#include <iostream>
#include <windows.h>
#include <commdlg.h>
#include "tinyfiledialogs.h"
#include "functiondec.h"

// GUI/windowing/rendering
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

// Global var :(
static bool showImageViewer = false;
static bool showPrereqPopup = false;
static std::string latestMessage = "";
static bool showObjectCntPopup = false;
std::string imageFilename;
cv::Mat originalImage, currentImage;
bool singleChannel = false;
bool isGrayscale = false;
bool isBinary = false;
bool cellsSegmented = false;

// TODO
// - implement 1 previous/next "edit" counter for Ctrl+Z

void OpenImage(GLuint& imageTexture, int& imageWidth, int& imageHeight) {
    const char* filter_patterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.tif", "*.tiff" };
    const char* file = tinyfd_openFileDialog(
        "Open Image",
        "",
        5,
        filter_patterns,
        "Image files",
        0
    );

    if (file) {
        imageFilename = std::string(file);

        cv::Mat img = cv::imread(file, cv::IMREAD_COLOR);
        if (img.empty()) {
            std::cerr << "Failed to load image." << std::endl;
        } else {
            originalImage = img.clone();
            currentImage = img.clone();
            cv::cvtColor(img, img, cv::COLOR_BGR2RGB); // OpenGL wants RGB
            //std::cout << "Channels: " << img.channels() << std::endl;
            imageWidth = img.cols;
            imageHeight = img.rows;

            if (imageTexture) glDeleteTextures(1, &imageTexture);
            glGenTextures(1, &imageTexture);
            glBindTexture(GL_TEXTURE_2D, imageTexture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);

            showImageViewer = true;
        }
    }
}

void UpdateTextureFromMat(const cv::Mat& img, GLuint& imageTexture, int& imageWidth, int& imageHeight) {
    //DEBUG
    /**
    if (img.empty() || img.channels() != 3 || img.type() != CV_8UC3) {
        std::cerr << "Invalid image format passed to UpdateTextureFromMat.\n";
        return;
    }
    */

    if (imageTexture) glDeleteTextures(1, &imageTexture);

    imageWidth = img.cols;
    imageHeight = img.rows;

    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DebugMatAndTexture(const cv::Mat& img, const std::string& context = "") {
    std::cout << "---- DebugMatAndTexture ----" << std::endl;
    if (!context.empty()) {
        std::cout << "Context: " << context << std::endl;
    }

    if (img.empty()) {
        std::cout << "Image is empty!" << std::endl;
        return;
    }

    std::cout << "Image size: " << img.cols << " x " << img.rows << std::endl;
    std::cout << "Channels: " << img.channels() << std::endl;
    std::cout << "Type: " << img.type() << std::endl;

    double minVal, maxVal;
    cv::minMaxLoc(img, &minVal, &maxVal);
    std::cout << "Min pixel value: " << minVal << ", Max pixel value: " << maxVal << std::endl;

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL Error after texture upload: 0x" << std::hex << err << std::dec << std::endl;
    }

    std::cout << "----------------------------" << std::endl;
}

std::string ShowSaveFileDialog(HWND owner = nullptr, const char* filter = "PNG Files\0*.png\0All Files\0*.*\0")
{
    char filename[MAX_PATH] = "";

    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;  
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "png";

    if (GetSaveFileName(&ofn))
        return std::string(filename);
    else
        return "";  // User canceled
}

int main()
{

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

    GLuint imageTexture = 0;
    int imageWidth = 0;
    int imageHeight = 0;

    int objectCount = 0;

    // Bools for GetKey
    bool openRequested = false;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ----------- KEYBINDS -----------//
        // Ctrl+O
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            openRequested = true;
        }
        if (openRequested) {
            openRequested = false;
            OpenImage(imageTexture, imageWidth, imageHeight);
        }
        // Ctrl+S
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
        // Ctrl+C
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
            cv::Mat tempBGRimg = originalImage.clone();
            //cv::imshow("before RGB2BGR", tempBGRimg);
            //cv::cvtColor(tempBGRimg, tempBGRimg, cv::COLOR_RGB2BGR); 
            //cv::imshow("after RGB2BGR", tempBGRimg);
            currentImage = showBlueChannelOnly(tempBGRimg);
            //DebugMatAndTexture(currentImage, "Before BGR2RGB");
            //cv::cvtColor(currentImage, currentImage, cv::COLOR_BGR2RGB);
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            singleChannel = true;
        }
        // Ctrl+G
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            currentImage = toGrayscale(currentImage.clone());
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            isGrayscale = true;
        }
        // Ctrl+B
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
            currentImage = gaussianFilter(currentImage.clone());
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            //isBlurred = true;
        }
        // Ctrl+P
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            currentImage = intensityThreshold(currentImage.clone());
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            isBinary = true;
        }
        // Ctrl+W
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            if (singleChannel && isGrayscale && isBinary) {
                WatershedOutput watershedOut = runWatershed(currentImage); 
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
        // Ctrl+1
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            currentImage = showBlueChannelOnly(currentImage);
            currentImage = toGrayscale(currentImage);
            currentImage = gaussianFilter(currentImage);
            currentImage = intensityThreshold(currentImage);
            
            WatershedOutput watershedOut = runWatershed(currentImage); 
            currentImage = watershedOut.watershedOutImg;
            objectCount = watershedOut.count;
            UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
            //std::cout << objectCount << std::endl;
            cellsSegmented = true;
            showObjectCntPopup = true;
        }

        //static bool showImageViewer = true;

        // ----------- MENU ITEMS -----------//
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
                ImGui::MenuItem("Undo", "TODO");
                ImGui::MenuItem("Redo", "TODO");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Image")) {
                if (ImGui::MenuItem("Isolate Channel", "Ctrl+C")) {
                    //cv::Mat tempBGRimg = originalImage.clone();
                    //cv::imshow("before RGB2BGR", tempBGRimg);
                    //cv::cvtColor(tempBGRimg, tempBGRimg, cv::COLOR_RGB2BGR); 
                    //cv::imshow("after RGB2BGR", tempBGRimg);
                    currentImage = showBlueChannelOnly(originalImage.clone());
                    //DebugMatAndTexture(currentImage, "Before BGR2RGB");
                    //cv::cvtColor(currentImage, currentImage, cv::COLOR_BGR2RGB);
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    singleChannel = true;
                }
                if (ImGui::MenuItem("Grayscale", "Ctrl+G")) {
                    currentImage = toGrayscale(currentImage.clone());
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    isGrayscale = true;
                }
                if (ImGui::MenuItem("Gaussian Blur", "Ctrl+B")) {
                    currentImage = gaussianFilter(currentImage.clone());
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    //isBlurred = true;
                }
                if (ImGui::MenuItem("Threshold Pixel Intensity", "Ctrl+P")) {
                    currentImage = intensityThreshold(currentImage.clone());
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    isBinary = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Analyze")) {
                if (ImGui::MenuItem("Object Count (Watershed only)", "Ctrl+W")) {
                    if (singleChannel && isGrayscale && isBinary) {
                        WatershedOutput watershedOut = runWatershed(currentImage); 
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
                if (ImGui::MenuItem("Object Count (pre-processing & Watershed)", "Ctrl+1")) {
                    currentImage = showBlueChannelOnly(currentImage);
                    currentImage = toGrayscale(currentImage);
                    currentImage = gaussianFilter(currentImage);
                    currentImage = intensityThreshold(currentImage);
                    
                    WatershedOutput watershedOut = runWatershed(currentImage); 
                    currentImage = watershedOut.watershedOutImg;
                    objectCount = watershedOut.count;
                    UpdateTextureFromMat(currentImage, imageTexture, imageWidth, imageHeight);
                    //std::cout << objectCount << std::endl;
                    cellsSegmented = true;
                    showObjectCntPopup = true;
                }
                ImGui::MenuItem("NSI Summary", "Ctrl+N");
                ImGui::MenuItem("NSI Heatmap", "Ctrl+H");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("README", "TODO");
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        
        }

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
    }

    // Cleanup
    if (imageTexture) glDeleteTextures(1, &imageTexture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    // ========== CHECK OPENGL VERSION (it is 3.3.0) ==========
    /**
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

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


