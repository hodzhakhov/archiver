#include <gtest/gtest.h>
#include "../src/processor/processor.h"
#include "../src/factory/factory.h"
#include "../src/compressor/compressor.h"
#include <fstream>
#include <filesystem>

class ArchiveProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "test_archive_data";
        std::filesystem::create_directories(test_dir);
        
        createTestFile("test1.txt", "Hello, World!");
        createTestFile("test2.txt", "This is a test file with more content.");
        createBinaryFile("binary.dat");
    }
    
    void TearDown() override {
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }
    
    void createTestFile(const std::string& name, const std::string& content) {
        std::ofstream file(test_dir + "/" + name);
        file << content;
        file.close();
    }
    
    void createBinaryFile(const std::string& name) {
        std::ofstream file(test_dir + "/" + name, std::ios::binary);
        for (int i = 0; i < 256; ++i) {
            file.put(static_cast<char>(i));
        }
        file.close();
    }
    
    std::vector<FileEntry> createTestFiles() {
        std::vector<FileEntry> files;
        
        FileEntry file1;
        file1.name = "test1.txt";
        file1.data = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
        files.push_back(file1);
        
        FileEntry file2;
        file2.name = "test2.txt";
        std::string content = "This is a test file with more content.";
        file2.data.assign(content.begin(), content.end());
        files.push_back(file2);
        
        FileEntry file3;
        file3.name = "binary.dat";
        for (int i = 0; i < 256; ++i) {
            file3.data.push_back(static_cast<uint8_t>(i));
        }
        files.push_back(file3);
        
        return files;
    }
    
    std::string test_dir;
};

TEST_F(ArchiveProcessorTest, CompressZipArchive) {
    ArchiveRequest request;
    request.operation = ArchiveOperation::COMPRESS;
    request.format = CompressionFormat::ZIP;
    request.archive_name = "test.zip";
    request.files = createTestFiles();

    auto compressor = CompressorFactory::createCompressor(CompressionFormat::ZIP);
    ArchiveProcessor processor(request, compressor);
    processor.process();
    
    EXPECT_EQ(processor.getOperation(), ArchiveOperation::COMPRESS);
    EXPECT_EQ(processor.getFormat(), CompressionFormat::ZIP);
    EXPECT_EQ(processor.getInputFilesCount(), 3);
    EXPECT_GT(processor.getOutputSize(), 0);
    EXPECT_NE(processor.getCompressionRatio(), 0.0);
    
    const auto& archive_data = processor.getArchiveData();
    EXPECT_FALSE(archive_data.empty());
}

TEST_F(ArchiveProcessorTest, CompressTarGzArchive) {
    ArchiveRequest request;
    request.operation = ArchiveOperation::COMPRESS;
    request.format = CompressionFormat::TAR_GZ;
    request.archive_name = "test.tar.gz";
    request.files = createTestFiles();
    
    auto compressor = CompressorFactory::createCompressor(CompressionFormat::TAR_GZ);
    ArchiveProcessor processor(request, compressor);
    processor.process();
    
    EXPECT_EQ(processor.getFormat(), CompressionFormat::TAR_GZ);
    EXPECT_GT(processor.getOutputSize(), 0);
    EXPECT_NE(processor.getCompressionRatio(), 0.0);
}

TEST_F(ArchiveProcessorTest, ExtractArchive) {
    ArchiveRequest compress_request;
    compress_request.operation = ArchiveOperation::COMPRESS;
    compress_request.format = CompressionFormat::ZIP;
    compress_request.archive_name = "test.zip";
    compress_request.files = createTestFiles();
    
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
    
    EXPECT_EQ(extract_processor.getOperation(), ArchiveOperation::EXTRACT);
    
    const auto& extracted_files = extract_processor.getExtractedFiles();
    EXPECT_EQ(extracted_files.size(), 3);
    
    bool found_test1 = false, found_test2 = false, found_binary = false;
    
    for (const auto& file : extracted_files) {
        if (file.name == "test1.txt") {
            found_test1 = true;
            std::string content(file.data.begin(), file.data.end());
            EXPECT_EQ(content, "Hello, World!");
        } else if (file.name == "test2.txt") {
            found_test2 = true;
            std::string content(file.data.begin(), file.data.end());
            EXPECT_EQ(content, "This is a test file with more content.");
        } else if (file.name == "binary.dat") {
            found_binary = true;
            EXPECT_EQ(file.data.size(), 256);
            for (size_t i = 0; i < file.data.size(); ++i) {
                EXPECT_EQ(file.data[i], static_cast<uint8_t>(i));
            }
        }
    }
    
    EXPECT_TRUE(found_test1);
    EXPECT_TRUE(found_test2);
    EXPECT_TRUE(found_binary);
}

TEST_F(ArchiveProcessorTest, ErrorHandling) {
    ArchiveRequest empty_request;
    empty_request.operation = ArchiveOperation::COMPRESS;
    empty_request.format = CompressionFormat::ZIP;
    empty_request.archive_name = "empty.zip";
    
    auto compressor = CompressorFactory::createCompressor(CompressionFormat::ZIP);
    EXPECT_THROW(ArchiveProcessor processor(empty_request, compressor), std::runtime_error);
    
    ArchiveRequest extract_empty;
    extract_empty.operation = ArchiveOperation::EXTRACT;
    extract_empty.format = CompressionFormat::ZIP;
    
    auto compressor2 = CompressorFactory::createCompressor(CompressionFormat::ZIP);
    EXPECT_THROW(ArchiveProcessor processor(extract_empty, compressor2), std::runtime_error);
}

TEST_F(ArchiveProcessorTest, ReprocessingError) {
    ArchiveRequest request;
    request.operation = ArchiveOperation::COMPRESS;
    request.format = CompressionFormat::ZIP;
    request.archive_name = "test.zip";
    request.files = createTestFiles();
    
    auto compressor = CompressorFactory::createCompressor(CompressionFormat::ZIP);
    ArchiveProcessor processor(request, compressor);
    processor.process();
    
    EXPECT_THROW(processor.process(), std::runtime_error);
}

TEST_F(ArchiveProcessorTest, DirectoryStructure) {
    std::vector<FileEntry> files;
    
    FileEntry root_file;
    root_file.name = "root.txt";
    root_file.data.assign({'r', 'o', 'o', 't'});
    files.push_back(root_file);
    
    FileEntry subdir_file;
    subdir_file.name = "subdir/file.txt";
    subdir_file.data.assign({'s', 'u', 'b'});
    files.push_back(subdir_file);
    
    FileEntry deep_file;
    deep_file.name = "dir1/dir2/deep.txt";
    deep_file.data.assign({'d', 'e', 'e', 'p'});
    files.push_back(deep_file);
    
    ArchiveRequest request;
    request.operation = ArchiveOperation::COMPRESS;
    request.format = CompressionFormat::ZIP;
    request.archive_name = "structure.zip";
    request.files = files;
    
    auto compressor = CompressorFactory::createCompressor(CompressionFormat::ZIP);
    ArchiveProcessor processor(request, compressor);
    processor.process();
    
    ArchiveRequest extract_request;
    extract_request.operation = ArchiveOperation::EXTRACT;
    extract_request.format = CompressionFormat::ZIP;
    extract_request.archive_data = processor.getArchiveData();
    
    auto compressor2 = CompressorFactory::createCompressor(CompressionFormat::ZIP);
    ArchiveProcessor extract_processor(extract_request, compressor2);
    extract_processor.process();
    
    const auto& extracted = extract_processor.getExtractedFiles();
    EXPECT_EQ(extracted.size(), 3);
    
    std::set<std::string> expected_names = {"root.txt", "subdir/file.txt", "dir1/dir2/deep.txt"};
    std::set<std::string> actual_names;
    
    for (const auto& file : extracted) {
        actual_names.insert(file.name);
    }
    
    EXPECT_EQ(expected_names, actual_names);
}

