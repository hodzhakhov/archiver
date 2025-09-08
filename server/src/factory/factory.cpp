#include "factory.h"
#include "../compressor/compressor.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>

std::shared_ptr<LibArchiveCompressor> CompressorFactory::createCompressor(CompressionFormat format) {
    return std::make_shared<LibArchiveCompressor>(format);
}

CompressionFormat CompressorFactory::formatFromString(const std::string& format_str) {
    std::string lower_format = format_str;
    std::transform(lower_format.begin(), lower_format.end(), lower_format.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    if (lower_format == "zip") return CompressionFormat::ZIP;
    if (lower_format == "tar.gz" || lower_format == "targz") return CompressionFormat::TAR_GZ;
    if (lower_format == "tar.bz2" || lower_format == "tarbz2") return CompressionFormat::TAR_BZ2;
    if (lower_format == "7z" || lower_format == "7zip") return CompressionFormat::SEVEN_Z;
    
    throw std::runtime_error("Unknown compression format: " + format_str);
}

std::string CompressorFactory::formatToString(CompressionFormat format) {
    switch (format) {
        case CompressionFormat::ZIP: return "zip";
        case CompressionFormat::TAR_GZ: return "tar.gz";
        case CompressionFormat::TAR_BZ2: return "tar.bz2";
        case CompressionFormat::SEVEN_Z: return "7z";
        default: return "unknown";
    }
}

std::vector<std::string> CompressorFactory::getSupportedFormats() {
    return {"zip", "tar.gz", "tar.bz2", "7z"};
}

bool CompressorFactory::isFormatSupported(const std::string& format_str) {
    try {
        formatFromString(format_str);
        return true;
    } catch (const std::runtime_error&) {
        return false;
    }
}
