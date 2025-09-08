#include "request_params.h"
#include "../../factory/factory.h"
#include <boost/json.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <stdexcept>

namespace json = boost::json;


std::vector<uint8_t> base64_decode(const std::string& encoded) {
    std::string decoded;
    decoded.resize(boost::beast::detail::base64::decoded_size(encoded.size()));
    
    auto result = boost::beast::detail::base64::decode(
        decoded.data(), 
        encoded.data(), 
        encoded.size()
    );
    
    decoded.resize(result.first);
    return std::vector<uint8_t>(decoded.begin(), decoded.end());
}


std::string base64_encode(const std::vector<uint8_t>& data) {
    std::string encoded;
    encoded.resize(boost::beast::detail::base64::encoded_size(data.size()));
    
    auto result = boost::beast::detail::base64::encode(
        encoded.data(),
        data.data(),
        data.size()
    );
    
    encoded.resize(result);
    return encoded;
}

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

        if (file_names.size() != file_contents.size()) {
            throw std::runtime_error("Number of file names and contents must match");
        }
        
        for (size_t i = 0; i < file_names.size(); ++i) {
            FileEntry file;
            file.name = file_names[i];
            

            try {
                file.data = base64_decode(file_contents[i]);
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to decode file content for " + file.name + ": " + e.what());
            }
            
            request.files.push_back(file);
        }
    } else {

        try {
            request.archive_data = base64_decode(archive_data_base64);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to decode archive data: " + std::string(e.what()));
        }
        request.extract_path = extract_path;
    }
    
    return request;
}

ArchiveRequestParams parse_json_body(const std::string& body) {
    ArchiveRequestParams params;

    try {
        json::value jv = json::parse(body);
        json::object obj = jv.as_object();


        params.operation = json::value_to<std::string>(obj.at("operation"));
        params.format = json::value_to<std::string>(obj.at("format"));
        

        if (obj.contains("archive_name")) {
            params.archive_name = json::value_to<std::string>(obj.at("archive_name"));
        }
        
        if (params.operation == "compress") {

            if (obj.contains("files")) {
                json::array files_arr = obj.at("files").as_array();
                for (const auto& file_val : files_arr) {
                    json::object file_obj = file_val.as_object();
                    
                    params.file_names.push_back(
                        json::value_to<std::string>(file_obj.at("name"))
                    );
                    params.file_contents.push_back(
                        json::value_to<std::string>(file_obj.at("content"))
                    );
                }
            }
        } else if (params.operation == "extract") {

            params.archive_data_base64 = json::value_to<std::string>(obj.at("archive_data"));
            
            if (obj.contains("extract_path")) {
                params.extract_path = json::value_to<std::string>(obj.at("extract_path"));
            }
        }
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid JSON body: " + std::string(e.what()));
    }


    if (params.operation != "compress" && params.operation != "extract") {
        throw std::runtime_error("Operation must be 'compress' or 'extract'");
    }
    
    if (!CompressorFactory::isFormatSupported(params.format)) {
        throw std::runtime_error("Unsupported format: " + params.format);
    }
    
    if (params.archive_name.empty() || params.archive_name == "archive.zip") {
        params.archive_name = "archive." + params.format;
    }
    
    if (params.operation == "compress" && params.file_names.empty()) {
        throw std::runtime_error("No files specified for compression");
    }
    
    if (params.operation == "extract" && params.archive_data_base64.empty()) {
        throw std::runtime_error("No archive data specified for extraction");
    }

    return params;
}