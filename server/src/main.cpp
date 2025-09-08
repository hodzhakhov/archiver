#include "server/server.h"
#include <iostream>

int main() {
    std::cout << "=== Archiver HTTP Server ===" << std::endl;
    std::cout << "Starting server on port 8080 with 4 threads..." << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  POST /archive - Archive operations (compress/extract)" << std::endl;
    std::cout << "  GET /formats  - List supported formats" << std::endl;
    std::cout << "=============================" << std::endl;
    
    try {
        start_server(8080, 4);
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}