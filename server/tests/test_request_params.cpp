#include <gtest/gtest.h>
#include "../src/server/request/request_params.h"
#include <boost/beast/core/detail/base64.hpp>

class RequestParamsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    
    std::string encodeBase64(const std::string& data) {
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
};

TEST_F(RequestParamsTest, ParseCompressRequest) {
    std::string json_body = R"({
        "operation": "compress",
        "format": "zip",
        "archive_name": "test.zip",
        "files": [
            {
                "name": "file1.txt",
                "content": ")" + encodeBase64("Hello, World!") + R"("
            },
            {
                "name": "file2.txt",
                "content": ")" + encodeBase64("Test content") + R"("
            }
        ]
    })";
    
    auto params = parse_json_body(json_body);
    
    EXPECT_EQ(params.operation, "compress");
    EXPECT_EQ(params.format, "zip");
    EXPECT_EQ(params.archive_name, "test.zip");
    EXPECT_EQ(params.file_names.size(), 2);
    EXPECT_EQ(params.file_contents.size(), 2);
    EXPECT_EQ(params.file_names[0], "file1.txt");
    EXPECT_EQ(params.file_names[1], "file2.txt");
    EXPECT_EQ(params.file_contents[0], encodeBase64("Hello, World!"));
    EXPECT_EQ(params.file_contents[1], encodeBase64("Test content"));
}

TEST_F(RequestParamsTest, ParseExtractRequest) {
    std::string archive_data = "fake_archive_data";
    std::string json_body = R"({
        "operation": "extract",
        "format": "zip",
        "archive_data": ")" + encodeBase64(archive_data) + R"(",
        "extract_path": "/tmp/extracted"
    })";
    
    auto params = parse_json_body(json_body);
    
    EXPECT_EQ(params.operation, "extract");
    EXPECT_EQ(params.format, "zip");
    EXPECT_EQ(params.archive_data_base64, encodeBase64(archive_data));
    EXPECT_EQ(params.extract_path, "/tmp/extracted");
}

TEST_F(RequestParamsTest, DefaultValues) {
    std::string json_body = R"({
        "operation": "compress",
        "format": "tar.gz",
        "files": [
            {
                "name": "test.txt",
                "content": ")" + encodeBase64("content") + R"("
            }
        ]
    })";
    
    auto params = parse_json_body(json_body);
    
    EXPECT_EQ(params.operation, "compress");
    EXPECT_EQ(params.format, "tar.gz");
    EXPECT_EQ(params.archive_name, "archive.tar.gz");
}

TEST_F(RequestParamsTest, ParseErrors) {
    EXPECT_THROW(parse_json_body("invalid json"), std::runtime_error);
    
    EXPECT_THROW(parse_json_body(R"({"format": "zip"})"), std::runtime_error);
    
    EXPECT_THROW(parse_json_body(R"({"operation": "compress"})"), std::runtime_error);
    
    EXPECT_THROW(parse_json_body(R"({"operation": "invalid", "format": "zip"})"), std::runtime_error);
    
    EXPECT_THROW(parse_json_body(R"({"operation": "compress", "format": "rar"})"), std::runtime_error);
    
    EXPECT_THROW(parse_json_body(R"({
        "operation": "compress",
        "format": "zip"
    })"), std::runtime_error);
    
    EXPECT_THROW(parse_json_body(R"({
        "operation": "extract",
        "format": "zip"
    })"), std::runtime_error);
}

TEST_F(RequestParamsTest, ToArchiveRequestCompress) {
    ArchiveRequestParams params;
    params.operation = "compress";
    params.format = "zip";
    params.archive_name = "test.zip";
    params.file_names = {"file1.txt", "file2.txt"};
    params.file_contents = {encodeBase64("content1"), encodeBase64("content2")};
    
    auto request = params.toArchiveRequest();
    
    EXPECT_EQ(request.operation, ArchiveOperation::COMPRESS);
    EXPECT_EQ(request.format, CompressionFormat::ZIP);
    EXPECT_EQ(request.archive_name, "test.zip");
    EXPECT_EQ(request.files.size(), 2);
    EXPECT_EQ(request.files[0].name, "file1.txt");
    EXPECT_EQ(request.files[1].name, "file2.txt");
    
    std::string content1(request.files[0].data.begin(), request.files[0].data.end());
    std::string content2(request.files[1].data.begin(), request.files[1].data.end());
    EXPECT_EQ(content1, "content1");
    EXPECT_EQ(content2, "content2");
}

TEST_F(RequestParamsTest, ToArchiveRequestExtract) {
    std::string archive_data = "test_archive_data";
    
    ArchiveRequestParams params;
    params.operation = "extract";
    params.format = "tar.gz";
    params.archive_data_base64 = encodeBase64(archive_data);
    params.extract_path = "/tmp/test";
    
    auto request = params.toArchiveRequest();
    
    EXPECT_EQ(request.operation, ArchiveOperation::EXTRACT);
    EXPECT_EQ(request.format, CompressionFormat::TAR_GZ);
    EXPECT_EQ(request.extract_path, "/tmp/test");
    
    std::string decoded_data(request.archive_data.begin(), request.archive_data.end());
    EXPECT_EQ(decoded_data, archive_data);
}

TEST_F(RequestParamsTest, ToArchiveRequestErrors) {
    ArchiveRequestParams params1;
    params1.operation = "unknown";
    params1.format = "zip";
    EXPECT_THROW(params1.toArchiveRequest(), std::runtime_error);
    
    ArchiveRequestParams params2;
    params2.operation = "compress";
    params2.format = "zip";
    params2.file_names = {"file1.txt", "file2.txt"};
    params2.file_contents = {"content1"};
    EXPECT_THROW(params2.toArchiveRequest(), std::runtime_error);
    
    ArchiveRequestParams params3;
    params3.operation = "compress";
    params3.format = "zip";
    params3.file_names = {"file1.txt"};
    params3.file_contents = {"@@@invalid_base64_with_invalid_chars!!!"};
    EXPECT_NO_THROW(params3.toArchiveRequest());
    
    ArchiveRequestParams params4;
    params4.operation = "extract";
    params4.format = "zip";
    params4.archive_data_base64 = "@@@invalid_base64_with_invalid_chars!!!";
    EXPECT_NO_THROW(params4.toArchiveRequest());
}

TEST_F(RequestParamsTest, DirectoryStructureInFiles) {
    std::string json_body = R"({
        "operation": "compress",
        "format": "zip",
        "archive_name": "structure.zip",
        "files": [
            {
                "name": "root.txt",
                "content": ")" + encodeBase64("root") + R"("
            },
            {
                "name": "dir/file.txt",
                "content": ")" + encodeBase64("subdir") + R"("
            },
            {
                "name": "dir1/dir2/deep.txt",
                "content": ")" + encodeBase64("deep") + R"("
            }
        ]
    })";
    
    auto params = parse_json_body(json_body);
    auto request = params.toArchiveRequest();
    
    EXPECT_EQ(request.files.size(), 3);
    EXPECT_EQ(request.files[0].name, "root.txt");
    EXPECT_EQ(request.files[1].name, "dir/file.txt");
    EXPECT_EQ(request.files[2].name, "dir1/dir2/deep.txt");
}
