#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

#include "tachyon/output/frame_output_sink.h"
#include <iostream>
#include <filesystem>
#include <vector>

namespace tachyon::output {

class EXRSequenceSink : public FrameOutputSink {
public:
    EXRSequenceSink() = default;

    bool begin(const RenderPlan& plan) override {
        m_output_dir = plan.output.destination.path;
        if (!std::filesystem::exists(m_output_dir)) {
            std::filesystem::create_directories(m_output_dir);
        }
        return true;
    }

    bool write_frame(const OutputFramePacket& packet) override {
        if (!packet.frame) return false;

        char buf[256];
        snprintf(buf, sizeof(buf), "frame_%05lld.exr", static_cast<long long>(packet.frame_number));
        std::filesystem::path full_path = std::filesystem::path(m_output_dir) / buf;

        const uint32_t width = packet.frame->width();
        const uint32_t height = packet.frame->height();
        
        EXRHeader header;
        InitEXRHeader(&header);

        EXRImage image;
        InitEXRImage(&image);

        image.num_channels = 4; // R, G, B, A for beauty
        for (const auto& aov : packet.aovs) {
            if (aov.name == "depth") image.num_channels += 1;
            else if (aov.name == "normal") image.num_channels += 3;
            else if (aov.name == "motion_vector") image.num_channels += 2;
            else image.num_channels += 1; // Unknown AOV as single channel
        }

        image.width = static_cast<int>(width);
        image.height = static_cast<int>(height);

        std::vector<float*> images(image.num_channels);
        std::vector<EXRChannelInfo> channels(image.num_channels);
        std::vector<int> pixel_types(image.num_channels, TINYEXR_PIXELTYPE_FLOAT);
        std::vector<int> requested_pixel_types(image.num_channels, TINYEXR_PIXELTYPE_HALF); // Save as HALF for size

        // Beauty Channels
        const auto& beauty_pixels = packet.frame->pixels();
        images[0] = new float[width * height]; // R
        images[1] = new float[width * height]; // G
        images[2] = new float[width * height]; // B
        images[3] = new float[width * height]; // A
        strncpy(channels[0].name, "R", 255);
        strncpy(channels[1].name, "G", 255);
        strncpy(channels[2].name, "B", 255);
        strncpy(channels[3].name, "A", 255);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const uint32_t idx = (y * width + x);
                const uint32_t src_idx = idx * 4;
                images[0][idx] = beauty_pixels[src_idx + 0];
                images[1][idx] = beauty_pixels[src_idx + 1];
                images[2][idx] = beauty_pixels[src_idx + 2];
                images[3][idx] = beauty_pixels[src_idx + 3];
            }
        }

        // AOV Channels
        int current_ch = 4;
        for (const auto& aov : packet.aovs) {
            if (aov.name == "depth") {
                images[current_ch] = new float[width * height];
                strncpy(channels[current_ch].name, "depth.Z", 255);
                for (uint32_t i = 0; i < width * height; ++i) images[current_ch][i] = aov.surface->pixels()[i * 4];
                current_ch++;
            } else if (aov.name == "normal") {
                images[current_ch] = new float[width * height];
                images[current_ch+1] = new float[width * height];
                images[current_ch+2] = new float[width * height];
                strncpy(channels[current_ch].name, "normal.X", 255);
                strncpy(channels[current_ch+1].name, "normal.Y", 255);
                strncpy(channels[current_ch+2].name, "normal.Z", 255);
                for (uint32_t i = 0; i < width * height; ++i) {
                    images[current_ch][i] = aov.surface->pixels()[i * 4 + 0];
                    images[current_ch+1][i] = aov.surface->pixels()[i * 4 + 1];
                    images[current_ch+2][i] = aov.surface->pixels()[i * 4 + 2];
                }
                current_ch += 3;
            } else if (aov.name == "motion_vector") {
                images[current_ch] = new float[width * height];
                images[current_ch+1] = new float[width * height];
                strncpy(channels[current_ch].name, "motion_vector.U", 255);
                strncpy(channels[current_ch+1].name, "motion_vector.V", 255);
                for (uint32_t i = 0; i < width * height; ++i) {
                    images[current_ch][i] = aov.surface->pixels()[i * 4 + 0];
                    images[current_ch+1][i] = aov.surface->pixels()[i * 4 + 1];
                }
                current_ch += 2;
            }
        }

        header.num_channels = image.num_channels;
        header.channels = channels.data();
        header.pixel_types = pixel_types.data();
        header.requested_pixel_types = requested_pixel_types.data();
        image.images = (unsigned char**)images.data();

        // Metadata as custom attributes
        std::vector<EXRAttribute> attributes;
        auto add_attr = [&](const char* name, const char* type, int size, const void* val) {
            EXRAttribute attr;
            strncpy(attr.name, name, 255);
            strncpy(attr.type, type, 255);
            attr.size = size;
            attr.value = (unsigned char*)val;
            attributes.push_back(attr);
        };

        add_attr("timecode", "string", (int)packet.metadata.timecode.size(), packet.metadata.timecode.c_str());
        add_attr("frameTime", "double", sizeof(double), &packet.metadata.time_seconds);
        add_attr("sceneHash", "string", (int)packet.metadata.scene_hash.size(), packet.metadata.scene_hash.c_str());
        add_attr("colorSpace", "string", (int)packet.metadata.color_space.size(), packet.metadata.color_space.c_str());

        header.num_custom_attributes = (int)attributes.size();
        header.custom_attributes = attributes.data();

        const char* err = nullptr;
        int ret = SaveEXRImageToFile(&image, &header, full_path.string().c_str(), &err);
        
        for (int i = 0; i < image.num_channels; ++i) delete[] images[i];

        if (ret != TINYEXR_SUCCESS) {
            m_last_error = err ? err : "Unknown EXR error";
            FreeEXRErrorMessage(err);
            return false;
        }

        return true;
    }

    bool finish() override { return true; }
    const std::string& last_error() const override { return m_last_error; }

private:
    std::string m_output_dir;
    std::string m_last_error;
};

std::unique_ptr<FrameOutputSink> create_exr_sequence_sink() {
    return std::make_unique<EXRSequenceSink>();
}

} // namespace tachyon::output
