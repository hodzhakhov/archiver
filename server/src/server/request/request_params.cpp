#include "request_params.h"
#include "../../factory/factory.h"
#include "../../processor/processor.h"
#include "multipart_parser.h"
#include <stdexcept>

ArchiveRequest ArchiveRequestParams::toArchiveRequest() const {
  ArchiveRequest request;

  if (operation == "compress") {
    request.operation = ArchiveOperation::COMPRESS;
  } else if (operation == "extract") {
    request.operation = ArchiveOperation::EXTRACT;
  } else {
    throw std::runtime_error("Unknown operation: " + operation);
  }

  request.format = CompressorFactory::formatFromString(format);
  request.archive_name = archive_name;

  if (request.operation == ArchiveOperation::COMPRESS) {
    request.files = files;
  } else {
    request.archive_data = archive_data;
    request.extract_path = extract_path;
  }

  return request;
}

ArchiveRequestParams parse_multipart_body(const std::string &body,
                                          const std::string &boundary) {
  ArchiveRequestParams params;

  try {
    MultipartFormData form_data = MultipartParser::parse(body, boundary);

    if (form_data.fields.find("operation") == form_data.fields.end()) {
      throw std::runtime_error("Missing required field: operation");
    }
    if (form_data.fields.find("format") == form_data.fields.end()) {
      throw std::runtime_error("Missing required field: format");
    }

    params.operation = form_data.fields.at("operation");
    params.format = form_data.fields.at("format");

    if (form_data.fields.find("archive_name") != form_data.fields.end()) {
      params.archive_name = form_data.fields.at("archive_name");
    }
    if (form_data.fields.find("extract_path") != form_data.fields.end()) {
      params.extract_path = form_data.fields.at("extract_path");
    }

    if (params.operation == "compress") {
      for (const auto &multipart_file : form_data.files) {
        FileEntry file;
        file.name = multipart_file.filename;
        file.data = multipart_file.data;
        params.files.push_back(file);
      }
    }

  } catch (const std::exception &e) {
    throw std::runtime_error("Invalid multipart body: " +
                             std::string(e.what()));
  }

  if (params.operation != "compress" && params.operation != "extract") {
    throw std::runtime_error("Operation must be 'compress' or 'extract'");
  }

  if (!CompressorFactory::isFormatSupported(params.format)) {
    throw std::runtime_error("Unsupported format: " + params.format);
  }

  if (params.archive_name.empty()) {
    params.archive_name = "archive." + params.format;
  }

  if (params.operation == "compress" && params.files.empty()) {
    throw std::runtime_error("No files specified for compression");
  }

  return params;
}

ArchiveRequestParams parse_archive_upload(const std::string &body) {
  ArchiveRequestParams params;
  params.operation = "extract";
  params.archive_data.assign(body.begin(), body.end());

  if (params.archive_data.empty()) {
    throw std::runtime_error("No archive data provided for extraction");
  }

  try {
    CompressionFormat detected_format = CompressorFactory::detectFormatFromData(params.archive_data);
    params.format = CompressorFactory::formatToString(detected_format);
  } catch (const std::exception& e) {
    params.format = "zip";
  }

  return params;
}
