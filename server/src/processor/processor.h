#pragma once

#include "../compressor/compressor.h"
#include <memory>
#include <string>
#include <vector>

enum class ArchiveOperation { COMPRESS, EXTRACT };

struct ArchiveRequest {
  ArchiveOperation operation;
  CompressionFormat format;
  std::string archive_name;
  std::vector<FileEntry> files;
  std::vector<uint8_t> archive_data;
  std::string extract_path;

  ArchiveRequest()
      : operation(ArchiveOperation::COMPRESS), format(CompressionFormat::ZIP) {}
};

class ArchiveProcessor {
public:
  ArchiveProcessor(const ArchiveRequest &request,
                   std::shared_ptr<LibArchiveCompressor> compressor);

  void process();

  const std::vector<uint8_t> &getArchiveData() const { return archive_data_; }
  const std::vector<FileEntry> &getExtractedFiles() const {
    return extracted_files_;
  }
  const std::string &getArchiveName() const { return request_.archive_name; }
  CompressionFormat getFormat() const { return request_.format; }
  ArchiveOperation getOperation() const { return request_.operation; }

  size_t getInputFilesCount() const { return request_.files.size(); }

private:
  ArchiveRequest request_;
  std::shared_ptr<LibArchiveCompressor> compressor_;
  std::vector<uint8_t> archive_data_;
  std::vector<FileEntry> extracted_files_;

  bool processed_ = false;

  void validateRequest();
  void performCompression();
  void performExtraction();
};
