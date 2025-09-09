#pragma once
#include <archive.h>
#include <archive_entry.h>
#include <string>
#include <vector>

enum class CompressionFormat { ZIP, TAR_GZ, TAR_BZ2, SEVEN_Z };

struct FileEntry {
  std::string name;
  std::string source_path;
  std::vector<uint8_t> data;

  FileEntry() = default;
  FileEntry(const std::string &name, const std::vector<uint8_t> &data)
      : name(name), data(data) {}
  FileEntry(const std::string &name, const std::string &source_path)
      : name(name), source_path(source_path) {}
};

class LibArchiveCompressor {
public:
  explicit LibArchiveCompressor(CompressionFormat format);
  ~LibArchiveCompressor() = default;

  std::vector<uint8_t> compress(const std::vector<FileEntry> &files);
  std::vector<FileEntry> extract(const std::vector<uint8_t> &archive_data);
  std::string getFormatName() const;
  std::string getFileExtension() const;

private:
  CompressionFormat format_;

  void setupArchiveFormat(struct archive *a) const;
  const char *getFormatString() const;
  std::vector<uint8_t> loadFileFromDisk(const std::string &path) const;
};
