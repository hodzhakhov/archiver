#include <gtest/gtest.h>
#include "../src/server/request/request_params.h"
#include "../src/server/request/multipart_parser.h"
#include "../src/processor/processor.h"

class RequestParamsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    
    std::string createMultipartBody(const std::string& boundary) {
        std::string body;
        body += "--" + boundary + "\r\n";
        body += "content-disposition: form-data; name=\"operation\"\r\n";
        body += "\r\n";
        body += "compress\r\n";
        
        body += "--" + boundary + "\r\n";
        body += "content-disposition: form-data; name=\"format\"\r\n";
        body += "\r\n";
        body += "zip\r\n";
        
        body += "--" + boundary + "\r\n";
        body += "content-disposition: form-data; name=\"archive_name\"\r\n";
        body += "\r\n";
        body += "test.zip\r\n";
        
        body += "--" + boundary + "\r\n";
        body += "content-disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n";
        body += "content-type: text/plain\r\n";
        body += "\r\n";
        body += "Hello, World!\r\n";
        
        body += "--" + boundary + "--\r\n";
        return body;
    }
    
    std::string createMultipartBodyWithMultipleFiles(const std::string& boundary) {
        std::string body;
        body += "--" + boundary + "\r\n";
        body += "content-disposition: form-data; name=\"operation\"\r\n";
        body += "\r\n";
        body += "compress\r\n";
        
        body += "--" + boundary + "\r\n";
        body += "content-disposition: form-data; name=\"format\"\r\n";
        body += "\r\n";
        body += "tar.gz\r\n";
        
        body += "--" + boundary + "\r\n";
        body += "content-disposition: form-data; name=\"archive_name\"\r\n";
        body += "\r\n";
        body += "multi.tar.gz\r\n";
        
        body += "--" + boundary + "\r\n";
        body += "content-disposition: form-data; name=\"file\"; filename=\"file1.txt\"\r\n";
        body += "content-type: text/plain\r\n";
        body += "\r\n";
        body += "Content of file 1\r\n";
        
        body += "--" + boundary + "\r\n";
        body += "content-disposition: form-data; name=\"file\"; filename=\"subdir/file2.txt\"\r\n";
        body += "content-type: text/plain\r\n";
        body += "\r\n";
        body += "Content of file 2 in subdirectory\r\n";
        
        body += "--" + boundary + "--\r\n";
        return body;
    }
};

TEST_F(RequestParamsTest, ParseMultipartCompressRequest) {
    std::string boundary = "----WebKitFormBoundaryTest123";
    std::string body = createMultipartBody(boundary);
    
    ArchiveRequestParams params = parse_multipart_body(body, boundary);
    
    EXPECT_EQ(params.operation, "compress");
    EXPECT_EQ(params.format, "zip");
    EXPECT_EQ(params.archive_name, "test.zip");
    EXPECT_EQ(params.files.size(), 1);
    EXPECT_EQ(params.files[0].name, "test.txt");
    
    std::string file_content(params.files[0].data.begin(), params.files[0].data.end());
    EXPECT_EQ(file_content, "Hello, World!");
}

TEST_F(RequestParamsTest, ParseMultipartWithMultipleFiles) {
    std::string boundary = "----WebKitFormBoundaryMultiple";
    std::string body = createMultipartBodyWithMultipleFiles(boundary);
    
    ArchiveRequestParams params = parse_multipart_body(body, boundary);
    
    EXPECT_EQ(params.operation, "compress");
    EXPECT_EQ(params.format, "tar.gz");
    EXPECT_EQ(params.archive_name, "multi.tar.gz");
    EXPECT_EQ(params.files.size(), 2);
    
    EXPECT_EQ(params.files[0].name, "file1.txt");
    std::string content1(params.files[0].data.begin(), params.files[0].data.end());
    EXPECT_EQ(content1, "Content of file 1");
    
    EXPECT_EQ(params.files[1].name, "subdir/file2.txt");
    std::string content2(params.files[1].data.begin(), params.files[1].data.end());
    EXPECT_EQ(content2, "Content of file 2 in subdirectory");
}

TEST_F(RequestParamsTest, ParseMultipartMissingOperation) {
    std::string boundary = "----WebKitFormBoundaryError";
    std::string body;
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"format\"\r\n";
    body += "\r\n";
    body += "zip\r\n";
    body += "--" + boundary + "--\r\n";
    
    EXPECT_THROW(parse_multipart_body(body, boundary), std::runtime_error);
}

TEST_F(RequestParamsTest, ParseMultipartMissingFormat) {
    std::string boundary = "----WebKitFormBoundaryError2";
    std::string body;
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"operation\"\r\n";
    body += "\r\n";
    body += "compress\r\n";
    body += "--" + boundary + "--\r\n";
    
    EXPECT_THROW(parse_multipart_body(body, boundary), std::runtime_error);
}

TEST_F(RequestParamsTest, ParseMultipartInvalidOperation) {
    std::string boundary = "----WebKitFormBoundaryInvalid";
    std::string body;
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"operation\"\r\n";
    body += "\r\n";
    body += "invalid_operation\r\n";
    
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"format\"\r\n";
    body += "\r\n";
    body += "zip\r\n";
    body += "--" + boundary + "--\r\n";
    
    EXPECT_THROW(parse_multipart_body(body, boundary), std::runtime_error);
}

TEST_F(RequestParamsTest, ParseMultipartUnsupportedFormat) {
    std::string boundary = "----WebKitFormBoundaryFormat";
    std::string body;
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"operation\"\r\n";
    body += "\r\n";
    body += "compress\r\n";
    
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"format\"\r\n";
    body += "\r\n";
    body += "unsupported_format\r\n";
    body += "--" + boundary + "--\r\n";
    
    EXPECT_THROW(parse_multipart_body(body, boundary), std::runtime_error);
}

TEST_F(RequestParamsTest, ParseMultipartNoFilesForCompression) {
    std::string boundary = "----WebKitFormBoundaryNoFiles";
    std::string body;
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"operation\"\r\n";
    body += "\r\n";
    body += "compress\r\n";
    
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"format\"\r\n";
    body += "\r\n";
    body += "zip\r\n";
    body += "--" + boundary + "--\r\n";
    
    EXPECT_THROW(parse_multipart_body(body, boundary), std::runtime_error);
}

TEST_F(RequestParamsTest, ParseArchiveUpload) {
    std::string archive_data = "PK\x03\x04test archive data";
    
    ArchiveRequestParams params = parse_archive_upload(archive_data);
    
    EXPECT_EQ(params.operation, "extract");
    EXPECT_EQ(params.format, "zip");
    EXPECT_EQ(params.archive_data.size(), archive_data.size());
    
    std::string parsed_data(params.archive_data.begin(), params.archive_data.end());
    EXPECT_EQ(parsed_data, archive_data);
}

TEST_F(RequestParamsTest, ParseArchiveUploadEmpty) {
    std::string empty_data = "";
    
    EXPECT_THROW(parse_archive_upload(empty_data), std::runtime_error);
}

TEST_F(RequestParamsTest, ParseArchiveUploadDetectFormats) {
    std::string zip_data = "PK\x03\x04test zip data";
    ArchiveRequestParams zip_params = parse_archive_upload(zip_data);
    EXPECT_EQ(zip_params.format, "zip");
    
    std::string gzip_data = "\x1F\x8Btest gzip data";
    ArchiveRequestParams gzip_params = parse_archive_upload(gzip_data);
    EXPECT_EQ(gzip_params.format, "tar.gz");
    
    std::string bzip2_data = "BZtest bzip2 data";
    ArchiveRequestParams bzip2_params = parse_archive_upload(bzip2_data);
    EXPECT_EQ(bzip2_params.format, "tar.bz2");
    
    std::string sevenz_data = "\x37\x7A\xBC\xAF\x27\x1Ctest 7z data";
    ArchiveRequestParams sevenz_params = parse_archive_upload(sevenz_data);
    EXPECT_EQ(sevenz_params.format, "7z");
    
    std::string unknown_data = "UNKNtest unknown data";
    ArchiveRequestParams unknown_params = parse_archive_upload(unknown_data);
    EXPECT_EQ(unknown_params.format, "zip");
}

TEST_F(RequestParamsTest, ToArchiveRequestCompress) {
    ArchiveRequestParams params;
    params.operation = "compress";
    params.format = "zip";
    params.archive_name = "test.zip";
    
    FileEntry file;
    file.name = "test.txt";
    file.data = {'H', 'e', 'l', 'l', 'o'};
    params.files.push_back(file);
    
    ArchiveRequest request = params.toArchiveRequest();
    
    EXPECT_EQ(request.operation, ArchiveOperation::COMPRESS);
    EXPECT_EQ(request.format, CompressionFormat::ZIP);
    EXPECT_EQ(request.archive_name, "test.zip");
    EXPECT_EQ(request.files.size(), 1);
    EXPECT_EQ(request.files[0].name, "test.txt");
}

TEST_F(RequestParamsTest, ToArchiveRequestExtract) {
    ArchiveRequestParams params;
    params.operation = "extract";
    params.format = "tar.gz";
    params.extract_path = "/tmp/extract";
    params.archive_data = {'P', 'K', 0x03, 0x04};
    
    ArchiveRequest request = params.toArchiveRequest();
    
    EXPECT_EQ(request.operation, ArchiveOperation::EXTRACT);
    EXPECT_EQ(request.format, CompressionFormat::TAR_GZ);
    EXPECT_EQ(request.extract_path, "/tmp/extract");
    EXPECT_EQ(request.archive_data.size(), 4);
}

TEST_F(RequestParamsTest, ToArchiveRequestInvalidOperation) {
    ArchiveRequestParams params;
    params.operation = "invalid";
    params.format = "zip";
    
    EXPECT_THROW(params.toArchiveRequest(), std::runtime_error);
}

TEST_F(RequestParamsTest, DefaultValues) {
    ArchiveRequestParams params;
    
    EXPECT_EQ(params.operation, "compress");
    EXPECT_EQ(params.format, "zip");
    EXPECT_EQ(params.archive_name, "archive.zip");
    EXPECT_TRUE(params.files.empty());
    EXPECT_TRUE(params.archive_data.empty());
    EXPECT_TRUE(params.extract_path.empty());
}

TEST_F(RequestParamsTest, ParseMultipartDefaultArchiveName) {
    std::string boundary = "----WebKitFormBoundaryDefault";
    std::string body;
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"operation\"\r\n";
    body += "\r\n";
    body += "compress\r\n";
    
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"format\"\r\n";
    body += "\r\n";
    body += "tar.bz2\r\n";
    
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n";
    body += "content-type: text/plain\r\n";
    body += "\r\n";
    body += "Test content\r\n";
    
    body += "--" + boundary + "--\r\n";
    
    ArchiveRequestParams params = parse_multipart_body(body, boundary);
    
    EXPECT_EQ(params.archive_name, "archive.zip");
}

TEST_F(RequestParamsTest, ParseMultipartBinaryFileData) {
    std::string boundary = "----WebKitFormBoundaryBinary";
    std::string body;
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"operation\"\r\n";
    body += "\r\n";
    body += "compress\r\n";
    
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"format\"\r\n";
    body += "\r\n";
    body += "zip\r\n";
    
    body += "--" + boundary + "\r\n";
    body += "content-disposition: form-data; name=\"file\"; filename=\"binary.bin\"\r\n";
    body += "content-type: application/octet-stream\r\n";
    body += "\r\n";
    
    std::string binary_data;
    for (int i = 0; i < 256; ++i) {
        binary_data += static_cast<char>(i);
    }
    body += binary_data + "\r\n";
    
    body += "--" + boundary + "--\r\n";
    
    ArchiveRequestParams params = parse_multipart_body(body, boundary);
    
    EXPECT_EQ(params.files.size(), 1);
    EXPECT_EQ(params.files[0].name, "binary.bin");
    EXPECT_EQ(params.files[0].data.size(), 256);
    
    for (size_t i = 0; i < params.files[0].data.size(); ++i) {
        EXPECT_EQ(params.files[0].data[i], static_cast<uint8_t>(i));
    }
}
