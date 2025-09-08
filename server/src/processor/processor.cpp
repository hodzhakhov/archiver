#include "processor.h"
#include <iostream>
#include <stdexcept>

ArchiveProcessor::ArchiveProcessor(const ArchiveRequest &request,
                                   std::shared_ptr<LibArchiveCompressor> compressor)
    : request_(request), compressor_(std::move(compressor)) {
  validateRequest();
}

void ArchiveProcessor::process() {
  if (processed_) {
    throw std::runtime_error("Archive processor has already been processed");
  }

  try {
    switch (request_.operation) {
    case ArchiveOperation::COMPRESS:
      performCompression();
      break;

    case ArchiveOperation::EXTRACT:
      performExtraction();
      break;

    default:
      throw std::runtime_error("Unknown archive operation");
    }

    processed_ = true;

  } catch (const std::exception &e) {
    std::cerr << "Archive processing error: " << e.what() << std::endl;
    throw;
  }
}

size_t ArchiveProcessor::getOutputSize() const {
  if (!processed_) {
    return 0;
  }

  switch (request_.operation) {
  case ArchiveOperation::COMPRESS:
    return archive_data_.size();

  case ArchiveOperation::EXTRACT: {
    size_t total_size = 0;
    for (const auto &file : extracted_files_) {
      total_size += file.data.size();
    }
    return total_size;
  }

  default:
    return 0;
  }
}

double ArchiveProcessor::getCompressionRatio() const {
  if (!processed_ || request_.operation != ArchiveOperation::COMPRESS) {
    return 0.0;
  }

  size_t input_size = calculateInputSize();
  size_t output_size = archive_data_.size();

  if (input_size == 0) {
    return 0.0;
  }

  return (1.0 -
          static_cast<double>(output_size) / static_cast<double>(input_size)) *
         100.0;
}

void ArchiveProcessor::validateRequest() {
  if (!compressor_) {
    throw std::runtime_error("Compressor is not initialized");
  }

  switch (request_.operation) {
  case ArchiveOperation::COMPRESS:
    if (request_.files.empty()) {
      throw std::runtime_error("No files specified for compression");
    }
    if (request_.archive_name.empty()) {
      throw std::runtime_error("Archive name is not specified");
    }
    break;

  case ArchiveOperation::EXTRACT:
    if (request_.archive_data.empty()) {
      throw std::runtime_error("No archive data specified for extraction");
    }
    break;

  default:
    throw std::runtime_error("Unknown archive operation");
  }
}

void ArchiveProcessor::performCompression() {
  std::cout << "Compressing " << request_.files.size() << " files into "
            << compressor_->getFormatName() << " archive..." << std::endl;

  size_t input_size = calculateInputSize();
  std::cout << "Total input size: " << input_size << " bytes" << std::endl;

  archive_data_ = compressor_->compress(request_.files);

  std::cout << "Compression completed. Archive size: " << archive_data_.size()
            << " bytes (ratio: " << getCompressionRatio() << "%)" << std::endl;
}

void ArchiveProcessor::performExtraction() {
  std::cout << "Extracting " << compressor_->getFormatName() << " archive ("
            << request_.archive_data.size() << " bytes)..." << std::endl;

  extracted_files_ = compressor_->extract(request_.archive_data);

  std::cout << "Extraction completed. Extracted " << extracted_files_.size()
            << " files (total size: " << getOutputSize() << " bytes)"
            << std::endl;

  for (const auto &file : extracted_files_) {
    std::cout << "  - " << file.name << " (" << file.data.size() << " bytes)"
              << std::endl;
  }
}

size_t ArchiveProcessor::calculateInputSize() const {
  size_t total_size = 0;
  for (const auto &file : request_.files) {
    total_size += file.data.size();
  }
  return total_size;
}
