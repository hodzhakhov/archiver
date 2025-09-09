#include "factory.h"
#include "../compressor/compressor.h"
#include <boost/algorithm/string.hpp>
#include <stdexcept>

std::shared_ptr<LibArchiveCompressor>
CompressorFactory::createCompressor(CompressionFormat format) {
  return std::make_shared<LibArchiveCompressor>(format);
}

CompressionFormat
CompressorFactory::formatFromString(const std::string &format_str) {
  std::string lower_format = boost::algorithm::to_lower_copy(format_str);

  if (lower_format == "zip")
    return CompressionFormat::ZIP;
  if (lower_format == "tar.gz" || lower_format == "targz")
    return CompressionFormat::TAR_GZ;
  if (lower_format == "tar.bz2" || lower_format == "tarbz2")
    return CompressionFormat::TAR_BZ2;
  if (lower_format == "7z" || lower_format == "7zip")
    return CompressionFormat::SEVEN_Z;

  throw std::runtime_error("Unknown compression format: " + format_str);
}

std::string CompressorFactory::formatToString(CompressionFormat format) {
  switch (format) {
  case CompressionFormat::ZIP:
    return "zip";
  case CompressionFormat::TAR_GZ:
    return "tar.gz";
  case CompressionFormat::TAR_BZ2:
    return "tar.bz2";
  case CompressionFormat::SEVEN_Z:
    return "7z";
  default:
    return "unknown";
  }
}

std::vector<std::string> CompressorFactory::getSupportedFormats() {
  return {"zip", "tar.gz", "tar.bz2", "7z"};
}

bool CompressorFactory::isFormatSupported(const std::string &format_str) {
  try {
    formatFromString(format_str);
    return true;
  } catch (const std::runtime_error &) {
    return false;
  }
}

CompressionFormat CompressorFactory::detectFormatFromData(const std::vector<uint8_t> &data) {
  if (data.size() < 4) {
    throw std::runtime_error("Archive data too small to detect format");
  }

  // ZIP format - starts with PK (0x504B)
  if (data[0] == 0x50 && data[1] == 0x4B) {
    return CompressionFormat::ZIP;
  }
  
  // 7z format - starts with 7z signature (0x377ABCAF271C)
  if (data.size() >= 6 && 
      data[0] == 0x37 && data[1] == 0x7A && data[2] == 0xBC && 
      data[3] == 0xAF && data[4] == 0x27 && data[5] == 0x1C) {
    return CompressionFormat::SEVEN_Z;
  }
  
  // GZIP format - starts with 0x1F8B (for .tar.gz)
  if (data[0] == 0x1F && data[1] == 0x8B) {
    return CompressionFormat::TAR_GZ;
  }
  
  // BZIP2 format - starts with BZ (0x425A) (for .tar.bz2)
  if (data[0] == 0x42 && data[1] == 0x5A) {
    return CompressionFormat::TAR_BZ2;
  }
  
  return CompressionFormat::ZIP;
}
