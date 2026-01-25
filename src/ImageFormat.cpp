#include "ImageFormat.h"

#include <glad/gl.h>

namespace OM3D {

ImageFormatGL image_format_to_gl(ImageFormat format) {
    switch(format) {
        case ImageFormat::RGBA8_UNORM:      return ImageFormatGL{ GL_RGBA, GL_RGBA8, GL_UNSIGNED_BYTE };
        case ImageFormat::RGBA8_sRGB:       return ImageFormatGL{ GL_RGBA, GL_SRGB8_ALPHA8, GL_UNSIGNED_BYTE };
        case ImageFormat::RGB8_UNORM:       return ImageFormatGL{ GL_RGB, GL_RGB8, GL_UNSIGNED_BYTE };
        case ImageFormat::RGB8_sRGB:        return ImageFormatGL{ GL_RGB, GL_SRGB8, GL_UNSIGNED_BYTE };
        case ImageFormat::RG16_UNORM:       return ImageFormatGL{ GL_RG, GL_RG16, GL_UNSIGNED_SHORT };
        case ImageFormat::RGBA16_FLOAT:     return ImageFormatGL{ GL_RGBA, GL_RGBA16F, GL_FLOAT };
        case ImageFormat::Depth32_FLOAT:    return ImageFormatGL{ GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT32F, GL_FLOAT };
    }

    FATAL("Unknown image format");
}

size_t image_format_to_size(ImageFormat format) {
    switch(format) {
        case ImageFormat::RGBA8_UNORM:
        case ImageFormat::RGBA8_sRGB:
            return sizeof(char) * 4;
        case ImageFormat::RGB8_UNORM:
        case ImageFormat::RGB8_sRGB:
            return sizeof(char) * 3;
        case ImageFormat::RG16_UNORM:
            return sizeof(short) * 2;
        case ImageFormat::RGBA16_FLOAT:
            return sizeof(short) * 4;
        case ImageFormat::Depth32_FLOAT:
            return sizeof(float);
    }

    FATAL("Unknown image format");
}

}
