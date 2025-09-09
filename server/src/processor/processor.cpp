#include "processor.h"
#include <iostream>
#include <stdexcept>

ArchiveProcessor::ArchiveProcessor(
    const ArchiveRequest &request,
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

  archive_data_ = compressor_->compress(request_.files);
  std::cout << "Compression completed. Archive size: " << archive_data_.size()
            << std::endl;
}

void ArchiveProcessor::performExtraction() {
  std::cout << "Extracting " << compressor_->getFormatName() << " archive ("
            << request_.archive_data.size() << " bytes)..." << std::endl;

  extracted_files_ = compressor_->extract(request_.archive_data);
  std::cout << "Extraction completed. Extracted " << extracted_files_.size()
            << " files" << std::endl;
}
