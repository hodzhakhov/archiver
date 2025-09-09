#pragma once

#include <map>
#include <string>
#include <vector>

struct FileEntry;

struct MultipartFile {
  std::string name;
  std::string filename;
  std::string content_type;
  std::vector<uint8_t> data;
};

struct MultipartFormData {
  std::map<std::string, std::string> fields;
  std::vector<MultipartFile> files;
};

class MultipartParser {
public:
  static MultipartFormData parse(const std::string &body,
                                 const std::string &boundary);
  static std::string
  createMultipartResponse(const std::vector<FileEntry> &files,
                          const std::string &boundary);

private:
  static std::string findBoundary(const std::string &content_type);
  static std::vector<std::string> split(const std::string &str,
                                        const std::string &delimiter);
  static std::string trim(const std::string &str);
  static std::map<std::string, std::string>
  parseHeaders(const std::string &headers);
  static std::string extractFilename(const std::string &disposition);
};
