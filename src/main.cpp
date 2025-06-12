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

// Global var :(
static bool showImageViewer = false;
std::string imageFilename;

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
            cv::cvtColor(img, img, cv::COLOR_BGR2RGB); // OpenGL wants RGB
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

        // Ctrl+O
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            openRequested = true;
        }
        if (openRequested) {
            openRequested = false;
            OpenImage(imageTexture, imageWidth, imageHeight);
        }

        //static bool showImageViewer = true;

        // Menu Bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                    OpenImage(imageTexture, imageWidth, imageHeight);
                }
                ImGui::MenuItem("Open Directory", "Ctrl+Shift+O");
                ImGui::MenuItem("Save", "Ctrl+S");
                ImGui::MenuItem("Exit", "Alt+F4");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                ImGui::MenuItem("Undo", "Ctrl+Z");
                ImGui::MenuItem("Redo", "Ctrl+Shift+Z");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("README", "Ctrl+H");
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        
        }

        if (showImageViewer) {
            if (ImGui::Begin(imageFilename.c_str(), &showImageViewer, ImGuiWindowFlags_HorizontalScrollbar)) {
                ImGui::Image((void*)(intptr_t)imageTexture, ImVec2((float)imageWidth, (float)imageHeight));
            }
            ImGui::End();
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


