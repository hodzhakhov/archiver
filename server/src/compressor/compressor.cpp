#include "compressor.h"
#include "archive.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

LibArchiveCompressor::LibArchiveCompressor(CompressionFormat format)
    : format_(format) {}

std::vector<uint8_t>
LibArchiveCompressor::compress(const std::vector<FileEntry> &files) {
  struct archive *a = archive_write_new();
  if (!a) {
    throw std::runtime_error("Failed to create archive");
  }

  try {
    setupArchiveFormat(a);

    std::string temp_filename =
        "/tmp/libarchive_temp_" + std::to_string(time(nullptr)) + ".archive";

    if (archive_write_open_filename(a, temp_filename.c_str()) != ARCHIVE_OK) {
      throw std::runtime_error("Failed to open archive for writing: " +
                               std::string(archive_error_string(a)));
    }

    for (const auto &file : files) {
      struct archive_entry *entry = archive_entry_new();

      std::vector<uint8_t> file_data = file.data;

      archive_entry_set_pathname(entry, file.name.c_str());
      archive_entry_set_size(entry, file_data.size());
      archive_entry_set_mode(entry, AE_IFREG | 0644);
      archive_entry_set_mtime(entry, time(nullptr), 0);

      if (archive_write_header(a, entry) != ARCHIVE_OK) {
        archive_entry_free(entry);
        throw std::runtime_error("Failed to write file header: " +
                                 std::string(archive_error_string(a)));
      }

      if (!file_data.empty()) {
        la_ssize_t bytes_written =
            archive_write_data(a, file_data.data(), file_data.size());
        if (bytes_written < 0) {
          archive_entry_free(entry);
          throw std::runtime_error("Failed to write file data: " +
                                   std::string(archive_error_string(a)));
        }
      }

      archive_entry_free(entry);
    }

    if (archive_write_close(a) != ARCHIVE_OK) {
      throw std::runtime_error("Failed to close archive: " +
                               std::string(archive_error_string(a)));
    }

    archive_write_free(a);

    std::vector<uint8_t> result = loadFileFromDisk(temp_filename);

    std::remove(temp_filename.c_str());

    return result;

  } catch (...) {
    archive_write_free(a);
    throw;
  }
}

std::vector<FileEntry>
LibArchiveCompressor::extract(const std::vector<uint8_t> &archive_data) {
  std::string temp_filename =
      "/tmp/libarchive_extract_" + std::to_string(time(nullptr)) + ".archive";
  std::ofstream temp_file(temp_filename, std::ios::binary);
  if (!temp_file) {
    throw std::runtime_error("Cannot create temporary file for extraction");
  }
  temp_file.write(reinterpret_cast<const char *>(archive_data.data()),
                  archive_data.size());
  temp_file.close();

  struct archive *a = archive_read_new();
  if (!a) {
    std::remove(temp_filename.c_str());
    throw std::runtime_error("Failed to create archive reader");
  }

  try {
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    if (archive_read_open_filename(a, temp_filename.c_str(), 65536) !=
        ARCHIVE_OK) {
      throw std::runtime_error("Failed to open archive for reading: " +
                               std::string(archive_error_string(a)));
    }

    std::vector<FileEntry> extracted_files;
    struct archive_entry *entry;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
      FileEntry file;
      file.name = archive_entry_pathname(entry);

      la_int64_t file_size = archive_entry_size(entry);
      if (file_size > 0) {
        file.data.resize(file_size);

        la_ssize_t bytes_read =
            archive_read_data(a, file.data.data(), file_size);
        if (bytes_read < 0) {
          throw std::runtime_error("Failed to read file data: " +
                                   std::string(archive_error_string(a)));
        }

        file.data.resize(bytes_read);
      }

      extracted_files.push_back(std::move(file));
    }

    archive_read_free(a);
    std::remove(temp_filename.c_str());
    return extracted_files;

  } catch (...) {
    archive_read_free(a);
    std::remove(temp_filename.c_str());
    throw;
  }
}

void LibArchiveCompressor::setupArchiveFormat(struct archive *a) const {
  switch (format_) {
  case CompressionFormat::ZIP:
    archive_write_set_format_zip(a);
    archive_write_zip_set_compression_deflate(a);
    break;

  case CompressionFormat::TAR_GZ:
    archive_write_set_format_gnutar(a);
    archive_write_add_filter_gzip(a);
    break;

  case CompressionFormat::TAR_BZ2:
    archive_write_set_format_gnutar(a);
    archive_write_add_filter_bzip2(a);
    break;

  case CompressionFormat::SEVEN_Z:
    archive_write_set_format_7zip(a);
    break;

  default:
    throw std::runtime_error("Unsupported compression format");
  }
}

std::string LibArchiveCompressor::getFormatName() const {
  return getFormatString();
}

std::string LibArchiveCompressor::getFileExtension() const {
  switch (format_) {
  case CompressionFormat::ZIP:
    return ".zip";
  case CompressionFormat::TAR_GZ:
    return ".tar.gz";
  case CompressionFormat::TAR_BZ2:
    return ".tar.bz2";
  case CompressionFormat::SEVEN_Z:
    return ".7z";
  default:
    return ".archive";
  }
}

const char *LibArchiveCompressor::getFormatString() const {
  switch (format_) {
  case CompressionFormat::ZIP:
    return "ZIP";
  case CompressionFormat::TAR_GZ:
    return "TAR.GZ";
  case CompressionFormat::TAR_BZ2:
    return "TAR.BZ2";
  case CompressionFormat::SEVEN_Z:
    return "7Z";
  default:
    return "UNKNOWN";
  }
}

std::vector<uint8_t>
LibArchiveCompressor::loadFileFromDisk(const std::string &path) const {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Cannot open file: " + path);
  }

  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> data(file_size);
  file.read(reinterpret_cast<char *>(data.data()), file_size);

  if (!file.good() && !file.eof()) {
    throw std::runtime_error("Error reading file: " + path);
  }

  return data;
}
