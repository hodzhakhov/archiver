#include "request_handler.h"
#include "request_params.h"
#include "../../processor/processor.h"
#include "../../writer/writer.h"
#include "../../factory/factory.h"
#include <mutex>
#include <unordered_map>

namespace {
    struct CacheEntry {
        http::response<http::string_body> response;
    };

    std::unordered_map<std::string, CacheEntry> request_cache;
    std::mutex cache_mutex;
}

http::response<http::string_body> handle_request(const http::request<http::string_body>& req) {
    http::response<http::string_body> resp;
    resp.version(11);
    resp.set(http::field::server, BOOST_BEAST_VERSION_STRING);

    std::string cache_key = std::string(req.method_string()) +
                           std::string(req.target()) +
                           req.body();

    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = request_cache.find(cache_key);
        if (it != request_cache.end()) {
            resp = it->second.response;
            resp.prepare_payload();
            return resp;
        }
    }

    try {
        if (req.method() == http::verb::post && req.target() == "/archive") {
            if (req[http::field::content_type] != "application/json") {
                resp.result(http::status::bad_request);
                resp.set(http::field::content_type, "application/json");
                resp.body() = R"({"error": "Content-Type must be application/json"})";
                return resp;
            }

            ArchiveRequestParams params = parse_json_body(req.body());
            ArchiveRequest archive_request = params.toArchiveRequest();

            auto compressor = CompressorFactory::createCompressor(archive_request.format);
            ArchiveProcessor processor(archive_request, compressor);
            processor.process();

            ArchiveWriter writer;
            writer.write(processor);

            resp.result(http::status::ok);
            
            if (archive_request.operation == ArchiveOperation::COMPRESS) {
                auto archive_data = writer.getBinaryData();
                
                resp.set(http::field::content_type, "application/octet-stream");
                resp.set(http::field::content_disposition,
                        "attachment; filename=\"" + archive_request.archive_name + "\"");
                resp.body() = std::string(archive_data.begin(), archive_data.end());
                
            } else if (archive_request.operation == ArchiveOperation::EXTRACT) {
                if (!archive_request.extract_path.empty()) {
                    writer.saveExtractedFiles(archive_request.extract_path);
                }
                
                resp.set(http::field::content_type, "application/json");
                resp.body() = writer.getMetadataJson();
            }
            
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
            resp.body() = R"({"error": "Endpoint not found. Available endpoints: POST /archive, GET /formats"})";
        }
    } catch (const std::exception& e) {
        resp.result(http::status::internal_server_error);
        resp.set(http::field::content_type, "application/json");
        resp.body() = R"({"error": ")" + std::string(e.what()) + "\"}";
    }

    resp.prepare_payload();

    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        request_cache[cache_key] = {resp};
    }

    return resp;
}