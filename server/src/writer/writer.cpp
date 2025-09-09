#include "writer.h"
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
}

void ArchiveWriter::clear() {
  binary_buffer_.clear();
  extracted_files_.clear();
}
