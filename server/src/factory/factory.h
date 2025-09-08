#pragma once
#include "../compressor/compressor.h"
#include <memory>
#include <string>

class CompressorFactory {
public:
    static std::shared_ptr<LibArchiveCompressor> createCompressor(CompressionFormat format);
    
    static CompressionFormat formatFromString(const std::string& format_str);
    
    static std::string formatToString(CompressionFormat format);
    
    static std::vector<std::string> getSupportedFormats();
    
    static bool isFormatSupported(const std::string& format_str);
};
