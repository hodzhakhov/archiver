#include <gtest/gtest.h>
#include "../src/writer/writer.h"
#include "../src/processor/processor.h"
#include "../src/compressor/compressor.h"
#include "../src/factory/factory.h"
#include <boost/json.hpp>
#include <filesystem>
#include <fstream>

class ArchiveWriterTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_output_dir = "test_writer_output";
        std::filesystem::create_directories(test_output_dir);
    }
    
    void TearDown() override {
        if (std::filesystem::exists(test_output_dir)) {
            std::filesystem::remove_all(test_output_dir);
        }
    }
    
    std::vector<FileEntry> createTestFiles() {
        std::vector<FileEntry> files;
        
        FileEntry file1;
        file1.name = "test1.txt";
        file1.data = {'H', 'e', 'l', 'l', 'o'};
        files.push_back(file1);
        
        FileEntry file2;
        file2.name = "test2.txt";
        std::string content = "World!";
        file2.data.assign(content.begin(), content.end());
        files.push_back(file2);
        
        return files;
    }
    
    ArchiveProcessor createTestCompressProcessor() {
        ArchiveRequest request;
        request.operation = ArchiveOperation::COMPRESS;
        request.format = CompressionFormat::ZIP;
        request.archive_name = "test.zip";
        request.files = createTestFiles();
        
        auto compressor = CompressorFactory::createCompressor(CompressionFormat::ZIP);
        ArchiveProcessor processor(request, compressor);
        processor.process();
        return processor;
    }
    
    ArchiveProcessor createTestExtractProcessor() {
        auto compress_processor = createTestCompressProcessor();
        
        ArchiveRequest extract_request;
        extract_request.operation = ArchiveOperation::EXTRACT;
        extract_request.format = CompressionFormat::ZIP;
        extract_request.archive_data = compress_processor.getArchiveData();
        
        auto compressor2 = CompressorFactory::createCompressor(CompressionFormat::ZIP);
        ArchiveProcessor extract_processor(extract_request, compressor2);
        extract_processor.process();
        return extract_processor;
    }
    
    std::string test_output_dir;
};

TEST_F(ArchiveWriterTest, WriteCompressResult) {
    auto processor = createTestCompressProcessor();
    
    ArchiveWriter writer;
    writer.write(processor);
    
    EXPECT_FALSE(writer.isEmpty());
    EXPECT_GT(writer.getDataSize(), 0);
    EXPECT_EQ(writer.getBinaryData().size(), writer.getDataSize());
    
    std::string metadata = writer.getMetadataJson();
    EXPECT_FALSE(metadata.empty());
    EXPECT_TRUE(metadata.find("\"operation\": \"compress\"") != std::string::npos);
    EXPECT_TRUE(metadata.find("\"format\": \"zip\"") != std::string::npos);
    EXPECT_TRUE(metadata.find("\"archive_name\": \"test.zip\"") != std::string::npos);
    EXPECT_TRUE(metadata.find("\"input_files_count\": 2") != std::string::npos);
    EXPECT_TRUE(metadata.find("\"success\": true") != std::string::npos);
}

TEST_F(ArchiveWriterTest, WriteExtractResult) {
    auto processor = createTestExtractProcessor();
    
    ArchiveWriter writer;
    writer.write(processor);
    
    EXPECT_TRUE(writer.isEmpty());
    
    std::string metadata = writer.getMetadataJson();
    EXPECT_FALSE(metadata.empty());
    EXPECT_TRUE(metadata.find("\"operation\": \"extract\"") != std::string::npos);
    EXPECT_TRUE(metadata.find("\"format\": \"zip\"") != std::string::npos);
    EXPECT_TRUE(metadata.find("\"extracted_files_count\": 2") != std::string::npos);
    EXPECT_TRUE(metadata.find("\"success\": true") != std::string::npos);
    EXPECT_TRUE(metadata.find("\"files\": [") != std::string::npos);
    EXPECT_TRUE(metadata.find("\"name\": \"test1.txt\"") != std::string::npos);
    EXPECT_TRUE(metadata.find("\"name\": \"test2.txt\"") != std::string::npos);
}

TEST_F(ArchiveWriterTest, SaveToFile) {
    auto processor = createTestCompressProcessor();
    
    ArchiveWriter writer;
    writer.write(processor);
    
    std::string filename = test_output_dir + "/test_archive.zip";
    writer.saveToFile(filename);
    
    EXPECT_TRUE(std::filesystem::exists(filename));
    
    auto file_size = std::filesystem::file_size(filename);
    EXPECT_EQ(file_size, writer.getDataSize());
    
    std::ifstream file(filename, std::ios::binary);
    std::vector<uint8_t> file_content((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
    EXPECT_EQ(file_content, writer.getBinaryData());
}

TEST_F(ArchiveWriterTest, SaveExtractedFiles) {
    auto processor = createTestExtractProcessor();
    
    ArchiveWriter writer;
    writer.write(processor);
    
    std::string extract_dir = test_output_dir + "/extracted";
    writer.saveExtractedFiles(extract_dir);
    
    EXPECT_TRUE(std::filesystem::exists(extract_dir + "/test1.txt"));
    EXPECT_TRUE(std::filesystem::exists(extract_dir + "/test2.txt"));
    
    std::ifstream file1(extract_dir + "/test1.txt");
    std::string content1((std::istreambuf_iterator<char>(file1)),
                         std::istreambuf_iterator<char>());
    EXPECT_EQ(content1, "Hello");
    
    std::ifstream file2(extract_dir + "/test2.txt");
    std::string content2((std::istreambuf_iterator<char>(file2)),
                         std::istreambuf_iterator<char>());
    EXPECT_EQ(content2, "World!");
}

TEST_F(ArchiveWriterTest, SaveFilesWithDirectories) {
    std::vector<FileEntry> files;
    
    FileEntry file1;
    file1.name = "root.txt";
    file1.data = {'r', 'o', 'o', 't'};
    files.push_back(file1);
    
    FileEntry file2;
    file2.name = "subdir/file.txt";
    file2.data = {'s', 'u', 'b'};
    files.push_back(file2);
    
    FileEntry file3;
    file3.name = "dir1/dir2/deep.txt";
    file3.data = {'d', 'e', 'e', 'p'};
    files.push_back(file3);
    
    ArchiveRequest compress_request;
    compress_request.operation = ArchiveOperation::COMPRESS;
    compress_request.format = CompressionFormat::ZIP;
    compress_request.archive_name = "dirs.zip";
    compress_request.files = files;
    
    auto compressor = CompressorFactory::createCompressor(CompressionFormat::ZIP);
    ArchiveProcessor compress_processor(compress_request, compressor);
    compress_processor.process();
    
    ArchiveRequest extract_request;
    extract_request.operation = ArchiveOperation::EXTRACT;
    extract_request.format = CompressionFormat::ZIP;
    extract_request.archive_data = compress_processor.getArchiveData();
    
    auto compressor2 = CompressorFactory::createCompressor(CompressionFormat::ZIP);
    ArchiveProcessor extract_processor(extract_request, compressor2);
    extract_processor.process();
    
    ArchiveWriter writer;
    writer.write(extract_processor);
    
    std::string extract_dir = test_output_dir + "/dirs_extracted";
    writer.saveExtractedFiles(extract_dir);
    
    EXPECT_TRUE(std::filesystem::exists(extract_dir + "/root.txt"));
    EXPECT_TRUE(std::filesystem::exists(extract_dir + "/subdir/file.txt"));
    EXPECT_TRUE(std::filesystem::exists(extract_dir + "/dir1/dir2/deep.txt"));
    
    std::ifstream root_file(extract_dir + "/root.txt");
    std::string root_content((std::istreambuf_iterator<char>(root_file)),
                             std::istreambuf_iterator<char>());
    EXPECT_EQ(root_content, "root");
    
    std::ifstream sub_file(extract_dir + "/subdir/file.txt");
    std::string sub_content((std::istreambuf_iterator<char>(sub_file)),
                            std::istreambuf_iterator<char>());
    EXPECT_EQ(sub_content, "sub");
    
    std::ifstream deep_file(extract_dir + "/dir1/dir2/deep.txt");
    std::string deep_content((std::istreambuf_iterator<char>(deep_file)),
                             std::istreambuf_iterator<char>());
    EXPECT_EQ(deep_content, "deep");
}

TEST_F(ArchiveWriterTest, ClearWriter) {
    auto processor = createTestCompressProcessor();
    
    ArchiveWriter writer;
    writer.write(processor);
    
    EXPECT_FALSE(writer.isEmpty());
    EXPECT_GT(writer.getDataSize(), 0);
    
    writer.clear();
    
    EXPECT_TRUE(writer.isEmpty());
    EXPECT_EQ(writer.getDataSize(), 0);
    EXPECT_TRUE(writer.getMetadataJson().empty());
}

TEST_F(ArchiveWriterTest, SaveErrors) {
    ArchiveWriter empty_writer;
    
    EXPECT_THROW(empty_writer.saveToFile(test_output_dir + "/empty.zip"), std::runtime_error);
    
    EXPECT_THROW(empty_writer.saveExtractedFiles(test_output_dir + "/empty"), std::runtime_error);
}

TEST_F(ArchiveWriterTest, JsonEscaping) {
    std::vector<FileEntry> files;
    
    FileEntry file;
    file.name = "file with \"quotes\" and \n newlines.txt";
    file.data = {'t', 'e', 's', 't'};
    files.push_back(file);
    
    ArchiveRequest compress_request;
    compress_request.operation = ArchiveOperation::COMPRESS;
    compress_request.format = CompressionFormat::ZIP;
    compress_request.archive_name = "special.zip";
    compress_request.files = files;
    
    auto compressor = CompressorFactory::createCompressor(CompressionFormat::ZIP);
    ArchiveProcessor compress_processor(compress_request, compressor);
    compress_processor.process();
    
    ArchiveRequest extract_request;
    extract_request.operation = ArchiveOperation::EXTRACT;
    extract_request.format = CompressionFormat::ZIP;
    extract_request.archive_data = compress_processor.getArchiveData();
    
    auto compressor2 = CompressorFactory::createCompressor(CompressionFormat::ZIP);
    ArchiveProcessor extract_processor(extract_request, compressor2);
    extract_processor.process();
    
    ArchiveWriter writer;
    writer.write(extract_processor);
    
    std::string metadata = writer.getMetadataJson();
    
    EXPECT_TRUE(metadata.find("\\\"") != std::string::npos);
    EXPECT_TRUE(metadata.find("\\n") != std::string::npos);
    
    EXPECT_NO_THROW(boost::json::parse(metadata));
}
