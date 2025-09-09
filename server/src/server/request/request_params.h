#pragma once

#include "../../compressor/compressor.h"
#include <string>
#include <vector>

enum class ArchiveOperation;
struct ArchiveRequest;

struct ArchiveRequestParams {
  std::string operation;
  std::string format;
  std::string archive_name;

  std::vector<FileEntry> files;

  std::vector<uint8_t> archive_data;
  std::string extract_path;

  ArchiveRequestParams()
      : operation("compress"), format("zip"), archive_name("archive.zip") {}

  ArchiveRequest toArchiveRequest() const;
};

struct MultipartFormData;

ArchiveRequestParams parse_multipart_body(const std::string &body,
                                          const std::string &boundary);
ArchiveRequestParams parse_archive_upload(const std::string &body);
