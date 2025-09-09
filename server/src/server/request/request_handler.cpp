#include "request_handler.h"
#include "../../factory/factory.h"
#include "../../processor/processor.h"
#include "../../writer/writer.h"
#include "multipart_parser.h"
#include "request_params.h"
#include <iostream>
#include <random>
#include <sstream>

std::string extract_boundary(const std::string &content_type) {
  size_t boundary_pos = content_type.find("boundary=");
  if (boundary_pos == std::string::npos) {
    throw std::runtime_error("No boundary found in Content-Type");
  }

  std::string boundary = content_type.substr(boundary_pos + 9);

  if (boundary.front() == '"' && boundary.back() == '"') {
    boundary = boundary.substr(1, boundary.length() - 2);
  }

  return boundary;
}

std::string generate_boundary() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 15);

  std::ostringstream oss;
  oss << "----CustomBoundary";
  for (int i = 0; i < 16; ++i) {
    oss << std::hex << dis(gen);
  }
  return oss.str();
}

http::response<http::string_body>
handle_request(const http::request<http::string_body> &req) {
  http::response<http::string_body> resp;
  resp.version(11);
  resp.set(http::field::server, BOOST_BEAST_VERSION_STRING);

  try {
    if (req.method() == http::verb::post &&
        req.target() == "/archive/compress") {

      std::string content_type = std::string(req[http::field::content_type]);
      if (content_type.find("multipart/form-data") == std::string::npos) {
        resp.result(http::status::bad_request);
        resp.set(http::field::content_type, "application/json");
        resp.body() =
            R"({"error": "Content-Type must be multipart/form-data"})";
        return resp;
      }

      std::string boundary = extract_boundary(content_type);
      ArchiveRequestParams params = parse_multipart_body(req.body(), boundary);
      ArchiveRequest archive_request = params.toArchiveRequest();

      auto compressor =
          CompressorFactory::createCompressor(archive_request.format);
      ArchiveProcessor processor(archive_request, compressor);
      processor.process();

      ArchiveWriter writer;
      writer.write(processor);

      resp.result(http::status::ok);
      auto archive_data = writer.getBinaryData();

      resp.set(http::field::content_type, "application/octet-stream");
      resp.set(http::field::content_disposition,
               "attachment; filename=\"" + archive_request.archive_name + "\"");
      resp.body() = std::string(archive_data.begin(), archive_data.end());
    } else if (req.method() == http::verb::post &&
               req.target() == "/archive/extract") {

      ArchiveRequestParams params = parse_archive_upload(req.body());
      ArchiveRequest archive_request = params.toArchiveRequest();

      auto compressor =
          CompressorFactory::createCompressor(archive_request.format);
      ArchiveProcessor processor(archive_request, compressor);
      processor.process();

      ArchiveWriter writer;
      writer.write(processor);

      resp.result(http::status::ok);

      std::string boundary = generate_boundary();
      auto extracted_files = processor.getExtractedFiles();

      resp.set(http::field::content_type,
               "multipart/form-data; boundary=" + boundary);
      resp.body() =
          MultipartParser::createMultipartResponse(extracted_files, boundary);

    } else if (req.method() == http::verb::get && req.target() == "/formats") {
      resp.result(http::status::ok);
      resp.set(http::field::content_type, "application/json");

      std::string formats_json = "{\"supported_formats\": [";
      auto formats = CompressorFactory::getSupportedFormats();
      for (size_t i = 0; i < formats.size(); ++i) {
        formats_json += "\"" + formats[i] + "\"";
        if (i < formats.size() - 1) {
          formats_json += ", ";
        }
      }
      formats_json += "]}";

      resp.body() = formats_json;

    } else {
      resp.result(http::status::not_found);
      resp.set(http::field::content_type, "application/json");
      resp.body() =
          R"({"error": "Endpoint not found. Available endpoints: POST /archive/compress, POST /archive/extract, GET /formats"})";
    }
  } catch (const std::exception &e) {
    std::cout << "Error: " << e.what() << "\n";
    resp.result(http::status::internal_server_error);
    resp.set(http::field::content_type, "application/json");
    resp.body() = R"({"error": ")" + std::string(e.what()) + "\"}";
  }

  resp.prepare_payload();

  return resp;
}
