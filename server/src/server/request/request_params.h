#pragma once

#include <string>
#include <vector>
#include "../../processor/processor.h"

struct ArchiveRequestParams {
    std::string operation;      
    std::string format;         
    std::string archive_name;   
    
    std::vector<std::string> file_names;     
    std::vector<std::string> file_contents;  
    
    std::string archive_data_base64;  
    std::string extract_path;         
    
    ArchiveRequestParams() : operation("compress"), format("zip"), archive_name("archive.zip") {}
    
    ArchiveRequest toArchiveRequest() const;
};

ArchiveRequestParams parse_json_body(const std::string& body);