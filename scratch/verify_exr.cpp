#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: verify_exr <input.exr>" << std::endl;
        return 1;
    }

    const char* filename = argv[1];
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    const char* err = nullptr;
    int ret = LoadEXRWithLayer(&image, &header, filename, nullptr, &err);
    if (ret != TINYEXR_SUCCESS) {
        std::cerr << "Error loading EXR: " << (err ? err : "unknown") << std::endl;
        FreeEXRErrorMessage(err);
        return 1;
    }

    std::cout << "EXR Validation: " << filename << " (" << image.width << "x" << image.height << ")" << std::endl;
    std::cout << "Channels (" << header.num_channels << "):" << std::endl;
    
    for (int i = 0; i < header.num_channels; ++i) {
        std::string name = header.channels[i].name;
        float* data = reinterpret_cast<float*>(image.images[i]);
        
        double sum = 0.0;
        double max_val = 0.0;
        int non_zero_count = 0;
        
        for (int p = 0; p < image.width * image.height; ++p) {
            float val = data[p];
            sum += val;
            if (val > max_val) max_val = val;
            if (std::abs(val) > 1e-6) non_zero_count++;
        }

        double avg = sum / (image.width * image.height);
        std::cout << "  [" << i << "] " << name << " -> Avg: " << avg << ", Max: " << max_val << ", Non-Zero: " << non_zero_count << "px";
        
        // Basic Heuristics
        if (name == "A" && avg < 0.01) std::cout << " [WARNING: Low Opacity]";
        if (name == "depth.Z" && max_val < 0.001) std::cout << " [WARNING: Flat Depth]";
        if (non_zero_count == 0) std::cout << " [CRITICAL: Empty Channel]";
        
        std::cout << std::endl;
    }

    std::cout << "\nAttributes (" << header.num_custom_attributes << "):" << std::endl;
    for (int i = 0; i < header.num_custom_attributes; ++i) {
        const auto& attr = header.custom_attributes[i];
        std::cout << "  " << attr.name << " (" << attr.type << "): ";
        if (std::string(attr.type) == "string") {
            std::cout << std::string((const char*)attr.value, attr.size);
        } else if (std::string(attr.type) == "double") {
            std::cout << *(double*)attr.value;
        }
        std::cout << std::endl;
    }

    FreeEXRHeader(&header);
    FreeEXRImage(&image);
    return 0;
}
