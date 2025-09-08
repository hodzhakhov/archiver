cmake_minimum_required(VERSION 3.30)
project(archiver_server)

set(CMAKE_CXX_STANDARD 20)
set(BOOST_ROOT "/opt/homebrew/Cellar/boost/1.89.0")

find_package(PkgConfig)
find_package(Boost REQUIRED COMPONENTS json)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif()

find_package(GTest REQUIRED)
if(GTest_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

if(APPLE)
    find_path(LIBARCHIVE_INCLUDE_DIRS 
        NAMES archive.h 
        PATHS 
            /opt/homebrew/include 
            /opt/homebrew/Cellar/libarchive/*/include
            /usr/local/include 
            /usr/local/Cellar/libarchive/*/include
    )
    find_library(LIBARCHIVE_LIBRARIES 
        NAMES archive 
        PATHS 
            /opt/homebrew/lib 
            /opt/homebrew/Cellar/libarchive/*/lib
            /usr/local/lib 
            /usr/local/Cellar/libarchive/*/lib
    )
    
    set(LIBARCHIVE_FOUND TRUE)
    message(STATUS "Found libarchive headers: ${LIBARCHIVE_INCLUDE_DIRS}")
    message(STATUS "Found libarchive library: ${LIBARCHIVE_LIBRARIES}")
    
elseif(PkgConfig_FOUND)
    pkg_check_modules(LIBARCHIVE libarchive)
else()
    find_path(LIBARCHIVE_INCLUDE_DIRS archive.h)
    find_library(LIBARCHIVE_LIBRARIES archive)
    set(LIBARCHIVE_FOUND TRUE)
endif()

add_executable(server src/main.cpp
        src/compressor/compressor.cpp
        src/compressor/compressor.h
        src/factory/factory.cpp
        src/factory/factory.h
        src/processor/processor.cpp
        src/processor/processor.h
        src/server/request/request_params.cpp
        src/server/request/request_params.h
        src/server/request/request_handler.cpp
        src/server/request/request_handler.h
        src/server/server.cpp
        src/server/server.h
        src/writer/writer.cpp
        src/writer/writer.h
)

target_include_directories(server 
    PRIVATE 
        ${BOOST_INCLUDE_DIRS}
        ${LIBARCHIVE_INCLUDE_DIRS}
)

target_link_libraries(server ${Boost_LIBRARIES} ${LIBARCHIVE_LIBRARIES})

# Создаем исполняемый файл для тестов
add_executable(tests
    tests/main_test.cpp
    tests/test_processor.cpp
    tests/test_factory.cpp
    tests/test_request_params.cpp
    tests/test_writer.cpp
    src/processor/processor.cpp
    src/writer/writer.cpp
    src/factory/factory.cpp
    src/compressor/compressor.cpp
    src/server/request/request_params.cpp
)

target_include_directories(tests 
    PRIVATE 
        ${BOOST_INCLUDE_DIRS}
        ${LIBARCHIVE_INCLUDE_DIRS}
        ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(tests 
    ${Boost_LIBRARIES} 
    ${LIBARCHIVE_LIBRARIES}
    GTest::GTest 
    GTest::Main
)
