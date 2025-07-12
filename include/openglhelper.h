// GUI/windowing/rendering
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

// Tableview
#include <fstream>


// ------------------------- //
// ------- Rendering ------- //
// ------------------------- //

void OpenImage(GLuint& imageTexture, int& imageWidth, int& imageHeight) {
    const char* filter_patterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.tif", "*.tiff" };
    const char* dialog_title = "Open Image";
    const char* default_path = ""; // current directory
    int filter_count = 6;
    const char* filter_description = "Image files";
    int allow_multiple_select = 0;

    const char* file = tinyfd_openFileDialog(
        dialog_title,
        default_path,
        filter_count,
        filter_patterns,
        filter_description,
        allow_multiple_select
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
    //DEBUGGING
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

// ------------------------- //
// ------ ^Rendering^ ------ //
// ------------------------- //




// ------------------------- //
// ----- Miscellaneous ----- //
// ------------------------- //

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

void ShowDataTableAndExport(bool* pOpen, std::vector<double>& data) {
    if (!pOpen || !(*pOpen)) return;

    ImGui::Begin("Data Table", pOpen); // window will close if *pOpen is set to false

    // Render the table
    if (ImGui::BeginTable("DataTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Index");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < data.size(); ++i) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%zu", i);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.6f", data[i]);
        }

        ImGui::EndTable();
    }

    if (ImGui::Button("Export to CSV")) {
        const char* filterPatterns[] = { "*.csv" };
        const char* savePath = tinyfd_saveFileDialog(
            "Save CSV File",
            "export.csv",
            1,
            filterPatterns,
            "CSV files"
        );

        if (savePath) {
            std::ofstream file(savePath);
            if (file.is_open()) {
                file << "Index,Value\n";
                for (size_t i = 0; i < data.size(); ++i)
                    file << i << "," << data[i] << "\n";
                file.close();
            }
        }
    }

    ImGui::SameLine();

    // Exit button
    if (ImGui::Button("Close Table")) {
        *pOpen = false;
    }

    ImGui::End();
}

// ------------------------- //
// ---- ^Miscellaneous^ ---- //
// ------------------------- //

