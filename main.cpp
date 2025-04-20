#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <limits>
#include <filesystem> // For directory and path operations (C++17)
#include <new>        // Include for std::nothrow
#include <windows.h>  // For WinAPI file dialog
#include <commdlg.h>  // For GetOpenFileName

// Define these before including the stb headers
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Ensure stb_image.h is accessible
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" // Ensure stb_image_write.h is accessible
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h" // Correct header name: ensure stb_image_resize.h is accessible

// --- Constants ---
const int TARGET_WIDTH = 360;
const int TARGET_HEIGHT = 180;
const std::string OUTPUT_DIR_NAME = "output";

// --- Structures (same as before) ---
struct ColorRGBA {
    uint8_t r, g, b, a;
    bool operator<(const ColorRGBA& other) const {
        if (r != other.r) return r < other.r;
        if (g != other.g) return g < other.g;
        if (b != other.b) return b < other.b;
        return a < other.a;
    }
    bool operator==(const ColorRGBA& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    bool operator!=(const ColorRGBA& other) const {
        return !(*this == other);
    }
};

struct BoundingBox {
    int top = -1, left = -1, bottom = -1, right = -1;
    int width() const { return (right >= left) ? (right - left + 1) : 0; }
    int height() const { return (bottom >= top) ? (bottom - top + 1) : 0; }
    bool isValid() const { return width() > 0 && height() > 0; }
};

// --- Core Image Processing Functions (minor refinements) ---

ColorRGBA findBackgroundColor(const unsigned char* pixels, int width, int height, int channels) {
    std::map<ColorRGBA, size_t> colorCounts; // Use size_t for potentially large counts
    size_t maxCount = 0;
    ColorRGBA backgroundColor = {255, 255, 255, 255}; // Default to white

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const unsigned char* p = pixels + (static_cast<size_t>(y) * width + x) * channels;
            ColorRGBA color;
            color.r = p[0];
            color.g = p[1];
            color.b = p[2];
            color.a = (channels == 4) ? p[3] : 255;

            // Slightly prioritize opaque colors for background detection if counts are equal?
            // Or maybe pixels near the border? For now, simple frequency is used.
            // Ignore fully transparent pixels
            if (channels == 4 && color.a == 0) continue;

            colorCounts[color]++;
            if (colorCounts[color] > maxCount) {
                maxCount = colorCounts[color];
                backgroundColor = color;
            }
        }
    }
    // Optional: Add more sophisticated background detection (e.g., checking border pixels)
    return backgroundColor;
}

BoundingBox findContentBoundingBox(const unsigned char* pixels, int width, int height, int channels, const ColorRGBA& bgColor) {
    BoundingBox bbox;
    bbox.top = height;
    bbox.left = width;
    bbox.bottom = -1; // Initialize to invalid state
    bbox.right = -1;
    bool contentFound = false;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const unsigned char* p = pixels + (static_cast<size_t>(y) * width + x) * channels;
            ColorRGBA pixelColor;
            pixelColor.r = p[0];
            pixelColor.g = p[1];
            pixelColor.b = p[2];
            pixelColor.a = (channels == 4) ? p[3] : 255;

            bool isBackground = (pixelColor == bgColor);
            if (channels == 4 && pixelColor.a < 10) { // Consider nearly transparent as background too
                 isBackground = true;
            }

            if (!isBackground) {
                contentFound = true;
                if (y < bbox.top) bbox.top = y;
                if (y > bbox.bottom) bbox.bottom = y;
                if (x < bbox.left) bbox.left = x;
                if (x > bbox.right) bbox.right = x;
            }
        }
    }

    if (!contentFound) {
         bbox.top = 0; bbox.left = 0; bbox.bottom = -1; bbox.right = -1; // Ensure width/height are 0 or less
    }

    return bbox;
}

unsigned char* trimImage(const unsigned char* originalPixels, int originalWidth, int channels, const BoundingBox& bbox, int& outWidth, int& outHeight) {
    outWidth = bbox.width();
    outHeight = bbox.height();

    if (!bbox.isValid()) {
        return nullptr;
    }

    size_t trimmedSize = static_cast<size_t>(outWidth) * outHeight * 4; // Always RGBA output
    unsigned char* trimmedPixels = new (std::nothrow) unsigned char[trimmedSize]; // Use nothrow to check allocation
    if (!trimmedPixels) {
         std::cerr << "Error: Failed to allocate memory for trimmed image." << std::endl;
         return nullptr;
    }


    for (int y = 0; y < outHeight; ++y) {
        for (int x = 0; x < outWidth; ++x) {
            int srcX = bbox.left + x;
            int srcY = bbox.top + y;

            const unsigned char* srcPixel = originalPixels + (static_cast<size_t>(srcY) * originalWidth + srcX) * channels;
            unsigned char* dstPixel = trimmedPixels + (static_cast<size_t>(y) * outWidth + x) * 4; // Dest is RGBA

            dstPixel[0] = srcPixel[0];
            dstPixel[1] = srcPixel[1];
            dstPixel[2] = srcPixel[2];
            dstPixel[3] = (channels == 4) ? srcPixel[3] : 255;
        }
    }
    return trimmedPixels;
}

unsigned char* resizeImage(const unsigned char* inputPixels, int inputWidth, int inputHeight, int& outWidth, int& outHeight) {
     double scaleFactor = std::min(static_cast<double>(TARGET_WIDTH) / inputWidth,
                                   static_cast<double>(TARGET_HEIGHT) / inputHeight);

     outWidth = static_cast<int>(std::round(inputWidth * scaleFactor));
     outHeight = static_cast<int>(std::round(inputHeight * scaleFactor));

     // Ensure minimum size
     outWidth = std::max(1, outWidth);
     outHeight = std::max(1, outHeight);

     size_t resizedSize = static_cast<size_t>(outWidth) * outHeight * 4; // RGBA output
     unsigned char* resizedPixels = new (std::nothrow) unsigned char[resizedSize];
     if (!resizedPixels) {
         std::cerr << "Error: Failed to allocate memory for resized image." << std::endl;
         return nullptr;
     }

     // Input is always RGBA from trimImage
     // Correct function name from stb_image_resize.h
     int resize_result = stbir_resize_uint8(inputPixels, inputWidth, inputHeight, 0,
                                            resizedPixels, outWidth, outHeight, 0,
                                            4); // 4 channels (RGBA)

     if (!resize_result) {
         std::cerr << "Error: Image resizing failed." << std::endl;
         delete[] resizedPixels;
         return nullptr;
     }

     return resizedPixels;
}


// --- File Dialog Function ---
std::vector<std::string> selectImageFiles() {
    OPENFILENAMEA ofn;       // Common dialog box structure (use A for ANSI)
    char szFile[8192] = {0}; // Buffer for file names (needs to be large for multi-select)

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // No owner window
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    // Filter for common image types
    ofn.lpstrFilter = "Image Files\0*.JPG;*.JPEG;*.PNG;*.BMP;*.TGA;*.GIF\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    // OFN_EXPLORER: Use modern explorer-style dialog
    // OFN_PATHMUSTEXIST, OFN_FILEMUSTEXIST: Ensure selection is valid
    // OFN_ALLOWMULTISELECT: Allow selecting multiple files
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    std::vector<std::string> selectedFiles;

    // Display the Open dialog box.
    if (GetOpenFileNameA(&ofn) == TRUE) {
        char* p = szFile;
        std::string dir = p; // First string is the directory
        p += dir.length() + 1; // Move past the directory and the null terminator

        if (*p == 0) { // Only one file selected (buffer contains full path)
            selectedFiles.push_back(dir); // In this case, 'dir' is the full path
        } else { // Multiple files selected
            while (*p) {
                std::string filename = p;
                p += filename.length() + 1; // Move past the filename and the null terminator
                // Combine directory and filename using filesystem::path for correctness
                try {
                     // Use canonical path to resolve potential relative paths from dialog
                    std::filesystem::path fullPath = std::filesystem::canonical(std::filesystem::path(dir) / filename);
                    selectedFiles.push_back(fullPath.string());
                } catch (const std::filesystem::filesystem_error& e) {
                     // Fallback if canonical fails (e.g. network path issues)
                     std::cerr << "Warning: Could not get canonical path for " << filename << " (" << e.what() << "). Using combined path." << std::endl;
                     std::filesystem::path fullPath = std::filesystem::path(dir) / filename;
                     selectedFiles.push_back(fullPath.string());
                }
            }
        }
    } else {
         DWORD error = CommDlgExtendedError();
         if (error != 0) { // An error occurred other than cancel
             std::cerr << "Warning: GetOpenFileName failed. Error code: " << error << std::endl;
         }
        // else: User likely cancelled the dialog, which is fine.
    }

    return selectedFiles;
}

// --- Main Function ---
int main() {
    // --- 1. Select Input Files ---
    std::vector<std::string> inputFiles = selectImageFiles();

    if (inputFiles.empty()) {
        std::cout << "No files selected or dialog cancelled. Exiting." << std::endl;
        return 0;
    }

    std::cout << "Selected " << inputFiles.size() << " file(s)." << std::endl;

    // --- 2. Create Output Directory ---
    std::filesystem::path outputDirPath = OUTPUT_DIR_NAME;
    try {
        if (!std::filesystem::exists(outputDirPath)) {
            if (std::filesystem::create_directory(outputDirPath)) {
                std::cout << "Created output directory: " << outputDirPath.string() << std::endl;
            } else {
                std::cerr << "Error: Failed to create output directory: " << outputDirPath.string() << std::endl;
                return 1;
            }
        } else if (!std::filesystem::is_directory(outputDirPath)) {
             std::cerr << "Error: '" << outputDirPath.string() << "' exists but is not a directory." << std::endl;
             return 1;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error creating/checking directory: " << e.what() << std::endl;
        return 1;
    }

    // --- 3. Process Each File ---
    int successCount = 0;
    int failCount = 0;
    for (const auto& inputPathStr : inputFiles) {
        std::cout << "\nProcessing: " << inputPathStr << std::endl;
        std::filesystem::path inputPath(inputPathStr);

        // --- 3a. Load Image ---
        int width = 0, height = 0, channels = 0;
        unsigned char *pixels = stbi_load(inputPathStr.c_str(), &width, &height, &channels, 0);

        if (!pixels) {
            std::cerr << "  Error loading image: " << stbi_failure_reason() << std::endl;
            failCount++;
            continue;
        }
         if (channels < 3) {
             std::cerr << "  Error: Image must have at least 3 channels (RGB)." << std::endl;
             stbi_image_free(pixels);
             failCount++;
             continue;
         }
        std::cout << "  Loaded: " << width << "x" << height << ", Channels: " << channels << std::endl;

        // --- 3b. Find Background & Bounding Box ---
        ColorRGBA bgColor = findBackgroundColor(pixels, width, height, channels);
        BoundingBox bbox = findContentBoundingBox(pixels, width, height, channels, bgColor);

        if (!bbox.isValid()) {
            std::cerr << "  Warning: No content found after trimming analysis. Skipping." << std::endl;
            stbi_image_free(pixels);
            failCount++; // Count as failure if no content is usable
            continue;
        }
         std::cout << "  Content Box: X=" << bbox.left << ", Y=" << bbox.top << ", W=" << bbox.width() << ", H=" << bbox.height() << std::endl;

        // --- 3c. Trim ---
        int trimmedWidth = 0, trimmedHeight = 0;
        unsigned char* trimmedPixels = trimImage(pixels, width, channels, bbox, trimmedWidth, trimmedHeight);
        stbi_image_free(pixels); // Free original pixels now

        if (!trimmedPixels) {
            std::cerr << "  Error during trimming. Skipping." << std::endl;
            failCount++;
            continue; // Error already printed in trimImage
        }
         std::cout << "  Trimmed Size: " << trimmedWidth << "x" << trimmedHeight << std::endl;

        // --- 3d. Resize ---
        int resizedWidth = 0, resizedHeight = 0;
        unsigned char* resizedPixels = resizeImage(trimmedPixels, trimmedWidth, trimmedHeight, resizedWidth, resizedHeight);
        delete[] trimmedPixels; // Free trimmed pixels now

        if (!resizedPixels) {
             std::cerr << "  Error during resizing. Skipping." << std::endl;
             failCount++;
             continue; // Error already printed in resizeImage
        }
         std::cout << "  Resized Size: " << resizedWidth << "x" << resizedHeight << std::endl;


        // --- 3e. Save Result ---
        std::filesystem::path outputFilename = inputPath.stem().string() + "_trimmed.png";
        std::filesystem::path outputPath = outputDirPath / outputFilename;
        std::string outputPathStr = outputPath.string();

        // Save as PNG (4 channels RGBA)
        int write_result = stbi_write_png(outputPathStr.c_str(), resizedWidth, resizedHeight, 4, resizedPixels, resizedWidth * 4);
        delete[] resizedPixels; // Free final resized pixels

        if (!write_result) {
            std::cerr << "  Error writing output image to " << outputPathStr << std::endl;
            failCount++;
        } else {
            std::cout << "  Successfully saved: " << outputPathStr << std::endl;
            successCount++;
        }
    }

    // --- 4. Final Report ---
    std::cout << "\n----------------------------------------" << std::endl;
    std::cout << "Processing Complete." << std::endl;
    std::cout << "  Successfully processed: " << successCount << " file(s)." << std::endl;
    std::cout << "  Failed/Skipped:       " << failCount << " file(s)." << std::endl;
    std::cout << "Output saved in '" << OUTPUT_DIR_NAME << "' directory." << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    //Optional: Pause at the end when run from explorer
    std::cout << "\nPress Enter to exit..." << std::endl;
    // Read characters until newline to prevent immediate exit if buffer has content
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');


    return 0;
}