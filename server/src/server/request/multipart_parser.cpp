#include "multipart_parser.h"
#include "../../compressor/compressor.h"
#include <sstream>

MultipartFormData MultipartParser::parse(const std::string &body,
                                         const std::string &boundary) {
  MultipartFormData result;

  std::string delimiter = "--" + boundary;
  std::string end_delimiter = "--" + boundary + "--";

  auto parts = split(body, delimiter);

  for (const auto &part : parts) {
    if (part.empty() || part == "--\r\n" || part == "--") {
      continue;
    }

    size_t header_end = part.find("\r\n\r\n");
    if (header_end == std::string::npos) {
      continue;
    }

    std::string headers_str = part.substr(0, header_end);
    std::string content = part.substr(header_end + 4);

    if (content.size() >= 2 && content.substr(content.size() - 2) == "\r\n") {
      content = content.substr(0, content.size() - 2);
    }

    auto headers = parseHeaders(headers_str);

    if (headers.find("content-disposition") != headers.end()) {
      std::string disposition = headers["content-disposition"];

      if (disposition.find("form-data") != std::string::npos) {
        if (disposition.find("filename=") != std::string::npos) {
          MultipartFile file;
          file.filename = extractFilename(disposition);
          file.name = file.filename;
          file.content_type = headers.count("content-type")
                                  ? headers["content-type"]
                                  : "application/octet-stream";
          file.data.assign(content.begin(), content.end());
          result.files.push_back(file);
        } else {
          size_t name_start = disposition.find("name=\"");
          if (name_start != std::string::npos) {
            name_start += 6;
            size_t name_end = disposition.find("\"", name_start);
            if (name_end != std::string::npos) {
              std::string field_name =
                  disposition.substr(name_start, name_end - name_start);
              result.fields[field_name] = content;
            }
          }
        }
      }
    }
  }

  return result;
}

std::string
MultipartParser::createMultipartResponse(const std::vector<FileEntry> &files,
                                         const std::string &boundary) {
  std::ostringstream response;

  for (const auto &file : files) {
    response << "--" << boundary << "\r\n";
    response << "content-disposition: form-data; name=\"file\"; filename=\""
             << file.name << "\"\r\n";
    response << "content-type: application/octet-stream\r\n";
    response << "\r\n";
    response.write(reinterpret_cast<const char *>(file.data.data()),
                   file.data.size());
    response << "\r\n";
  }

  response << "--" << boundary << "--\r\n";

  return response.str();
}

std::vector<std::string> MultipartParser::split(const std::string &str,
                                                const std::string &delimiter) {
  std::vector<std::string> result;
  size_t start = 0;
  size_t end = str.find(delimiter);

  while (end != std::string::npos) {
    result.push_back(str.substr(start, end - start));
    start = end + delimiter.length();
    end = str.find(delimiter, start);
  }

  result.push_back(str.substr(start));
  return result;
}

std::string MultipartParser::trim(const std::string &str) {
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";

  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

std::map<std::string, std::string>
MultipartParser::parseHeaders(const std::string &headers) {
  std::map<std::string, std::string> result;
  std::istringstream stream(headers);
  std::string line;

  while (std::getline(stream, line)) {
    line = trim(line);
    if (line.empty())
      continue;

    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos) {
      std::string key = trim(line.substr(0, colon_pos));
      std::string value = trim(line.substr(colon_pos + 1));
      result[key] = value;
    }
  }

  return result;
}

std::string MultipartParser::extractFilename(const std::string &disposition) {
  size_t filename_start = disposition.find("filename=\"");
  if (filename_start == std::string::npos) {
    return "unknown";
  }

  filename_start += 10;
  size_t filename_end = disposition.find("\"", filename_start);
  if (filename_end == std::string::npos) {
    return "unknown";
  }

  return disposition.substr(filename_start, filename_end - filename_start);
}
