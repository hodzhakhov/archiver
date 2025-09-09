#include "server/server.h"
#include <iostream>

int main() {
    std::cout << "Starting server on port 8080" << std::endl;
    
    try {
        start_server(8080, 4);
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
