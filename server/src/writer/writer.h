#pragma once
#include "../processor/processor.h"
#include <vector>

class ArchiveWriter {
public:
  ArchiveWriter() = default;

  void write(const ArchiveProcessor &processor);

  std::vector<uint8_t> getBinaryData() const { return binary_buffer_; }

  size_t getDataSize() const { return binary_buffer_.size(); }

  void clear();

  bool isEmpty() const { return binary_buffer_.empty(); }

private:
  std::vector<uint8_t> binary_buffer_;
  std::vector<FileEntry> extracted_files_;
};
