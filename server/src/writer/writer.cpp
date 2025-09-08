#include "writer.h"
#include "../factory/factory.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

void ArchiveWriter::write(const ArchiveProcessor &processor) {
  clear();

  switch (processor.getOperation()) {
  case ArchiveOperation::COMPRESS:
    binary_buffer_ = processor.getArchiveData();
    break;

  case ArchiveOperation::EXTRACT:
    extracted_files_ = processor.getExtractedFiles();
    break;

  default:
    throw std::runtime_error("Unknown archive operation");
  }

  generateMetadata(processor);
}

void ArchiveWriter::saveToFile(const std::string &filename) const {
  if (binary_buffer_.empty()) {
    throw std::runtime_error("No archive data to save");
  }

  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Cannot create file: " + filename);
  }

  file.write(reinterpret_cast<const char *>(binary_buffer_.data()),
             binary_buffer_.size());

  if (!file.good()) {
    throw std::runtime_error("Error writing to file: " + filename);
  }

  std::cout << "Archive saved to: " << filename << " (" << binary_buffer_.size()
            << " bytes)" << std::endl;
}

void ArchiveWriter::saveExtractedFiles(
    const std::string &base_directory) const {
  if (extracted_files_.empty()) {
    throw std::runtime_error("No extracted files to save");
  }

  std::filesystem::create_directories(base_directory);

  for (const auto &file : extracted_files_) {
    std::string full_path = base_directory + "/" + file.name;

    std::filesystem::path file_path(full_path);
    if (file_path.has_parent_path()) {
      std::filesystem::create_directories(file_path.parent_path());
    }

    std::ofstream output_file(full_path, std::ios::binary);
    if (!output_file) {
      throw std::runtime_error("Cannot create file: " + full_path);
    }

    output_file.write(reinterpret_cast<const char *>(file.data.data()),
                      file.data.size());

    if (!output_file.good()) {
      throw std::runtime_error("Error writing to file: " + full_path);
    }

    std::cout << "Extracted file saved: " << full_path << " ("
              << file.data.size() << " bytes)" << std::endl;
  }
}

void ArchiveWriter::clear() {
  binary_buffer_.clear();
  metadata_json_.clear();
  extracted_files_.clear();
}

void ArchiveWriter::generateMetadata(const ArchiveProcessor &processor) {
  std::ostringstream json;

  json << "{\n";
  json << "  \"operation\": \""
       << (processor.getOperation() == ArchiveOperation::COMPRESS ? "compress"
                                                                  : "extract")
       << "\",\n";
  json << "  \"format\": \""
       << CompressorFactory::formatToString(processor.getFormat()) << "\",\n";
  json << "  \"archive_name\": \""
       << escapeJsonString(processor.getArchiveName()) << "\",\n";

  if (processor.getOperation() == ArchiveOperation::COMPRESS) {
    json << "  \"input_files_count\": " << processor.getInputFilesCount()
         << ",\n";
    json << "  \"archive_size\": " << processor.getOutputSize() << ",\n";
    json << "  \"compression_ratio\": " << processor.getCompressionRatio()
         << ",\n";
    json << "  \"success\": true\n";
  } else {
    json << "  \"extracted_files_count\": " << extracted_files_.size() << ",\n";
    json << "  \"total_extracted_size\": " << processor.getOutputSize()
         << ",\n";
    json << "  \"files\": [\n";

    for (size_t i = 0; i < extracted_files_.size(); ++i) {
      json << "    {\n";
      json << "      \"name\": \"" << escapeJsonString(extracted_files_[i].name)
           << "\",\n";
      json << "      \"size\": " << extracted_files_[i].data.size() << "\n";
      json << "    }";
      if (i < extracted_files_.size() - 1) {
        json << ",";
      }
      json << "\n";
    }

    json << "  ],\n";
    json << "  \"success\": true\n";
  }

  json << "}";

  metadata_json_ = json.str();
}

std::string ArchiveWriter::escapeJsonString(const std::string &str) const {
  std::string escaped;
  escaped.reserve(str.length());

  for (char c : str) {
    switch (c) {
    case '"':
      escaped += "\\\"";
      break;
    case '\\':
      escaped += "\\\\";
      break;
    case '\b':
      escaped += "\\b";
      break;
    case '\f':
      escaped += "\\f";
      break;
    case '\n':
      escaped += "\\n";
      break;
    case '\r':
      escaped += "\\r";
      break;
    case '\t':
      escaped += "\\t";
      break;
    default:
      if (c >= 0 && c < 0x20) {
        escaped += "\\u";
        char buf[5];
        snprintf(buf, sizeof(buf), "%04x", static_cast<unsigned char>(c));
        escaped += buf;
      } else {
        escaped += c;
      }
      break;
    }
  }

  return escaped;
}
