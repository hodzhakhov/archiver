#pragma once
#include "../processor/processor.h"
#include <string>
#include <vector>

class ArchiveWriter {
public:
  ArchiveWriter() = default;

  void write(const ArchiveProcessor &processor);

  std::vector<uint8_t> getBinaryData() const { return binary_buffer_; }

  size_t getDataSize() const { return binary_buffer_.size(); }

  std::string getMetadataJson() const { return metadata_json_; }

  void saveToFile(const std::string &filename) const;

  void saveExtractedFiles(const std::string &base_directory) const;

  void clear();

  bool isEmpty() const { return binary_buffer_.empty(); }

private:
  std::vector<uint8_t> binary_buffer_;
  std::string metadata_json_;
  std::vector<FileEntry> extracted_files_;

  void generateMetadata(const ArchiveProcessor &processor);

  std::string escapeJsonString(const std::string &str) const;
};
