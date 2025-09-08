#include <gtest/gtest.h>
#include "../src/factory/factory.h"
#include "../src/compressor/compressor.h"

class CompressorFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CompressorFactoryTest, CreateCompressors) {
    auto zip_compressor = CompressorFactory::createCompressor(CompressionFormat::ZIP);
    ASSERT_NE(zip_compressor, nullptr);
    EXPECT_EQ(zip_compressor->getFormatName(), "ZIP");
    EXPECT_EQ(zip_compressor->getFileExtension(), ".zip");
    
    auto targz_compressor = CompressorFactory::createCompressor(CompressionFormat::TAR_GZ);
    ASSERT_NE(targz_compressor, nullptr);
    EXPECT_EQ(targz_compressor->getFormatName(), "TAR.GZ");
    EXPECT_EQ(targz_compressor->getFileExtension(), ".tar.gz");
    
    auto tarbz2_compressor = CompressorFactory::createCompressor(CompressionFormat::TAR_BZ2);
    ASSERT_NE(tarbz2_compressor, nullptr);
    EXPECT_EQ(tarbz2_compressor->getFormatName(), "TAR.BZ2");
    EXPECT_EQ(tarbz2_compressor->getFileExtension(), ".tar.bz2");
    
    auto sevenz_compressor = CompressorFactory::createCompressor(CompressionFormat::SEVEN_Z);
    ASSERT_NE(sevenz_compressor, nullptr);
    EXPECT_EQ(sevenz_compressor->getFormatName(), "7Z");
    EXPECT_EQ(sevenz_compressor->getFileExtension(), ".7z");
}

TEST_F(CompressorFactoryTest, FormatFromString) {
    EXPECT_EQ(CompressorFactory::formatFromString("zip"), CompressionFormat::ZIP);
    EXPECT_EQ(CompressorFactory::formatFromString("ZIP"), CompressionFormat::ZIP);
    EXPECT_EQ(CompressorFactory::formatFromString("ZiP"), CompressionFormat::ZIP);
    
    EXPECT_EQ(CompressorFactory::formatFromString("tar.gz"), CompressionFormat::TAR_GZ);
    EXPECT_EQ(CompressorFactory::formatFromString("TAR.GZ"), CompressionFormat::TAR_GZ);
    EXPECT_EQ(CompressorFactory::formatFromString("targz"), CompressionFormat::TAR_GZ);
    
    EXPECT_EQ(CompressorFactory::formatFromString("tar.bz2"), CompressionFormat::TAR_BZ2);
    EXPECT_EQ(CompressorFactory::formatFromString("tarbz2"), CompressionFormat::TAR_BZ2);
    
    EXPECT_EQ(CompressorFactory::formatFromString("7z"), CompressionFormat::SEVEN_Z);
    EXPECT_EQ(CompressorFactory::formatFromString("7zip"), CompressionFormat::SEVEN_Z);
    
    EXPECT_THROW(CompressorFactory::formatFromString("unknown"), std::runtime_error);
    EXPECT_THROW(CompressorFactory::formatFromString(""), std::runtime_error);
    EXPECT_THROW(CompressorFactory::formatFromString("rar"), std::runtime_error);
}

TEST_F(CompressorFactoryTest, FormatToString) {
    EXPECT_EQ(CompressorFactory::formatToString(CompressionFormat::ZIP), "zip");
    EXPECT_EQ(CompressorFactory::formatToString(CompressionFormat::TAR_GZ), "tar.gz");
    EXPECT_EQ(CompressorFactory::formatToString(CompressionFormat::TAR_BZ2), "tar.bz2");
    EXPECT_EQ(CompressorFactory::formatToString(CompressionFormat::SEVEN_Z), "7z");
}

TEST_F(CompressorFactoryTest, GetSupportedFormats) {
    auto formats = CompressorFactory::getSupportedFormats();
    
    EXPECT_GT(formats.size(), 0);
    EXPECT_TRUE(std::find(formats.begin(), formats.end(), "zip") != formats.end());
    EXPECT_TRUE(std::find(formats.begin(), formats.end(), "tar.gz") != formats.end());
    EXPECT_TRUE(std::find(formats.begin(), formats.end(), "tar.bz2") != formats.end());
    EXPECT_TRUE(std::find(formats.begin(), formats.end(), "7z") != formats.end());
}

TEST_F(CompressorFactoryTest, IsFormatSupported) {
    EXPECT_TRUE(CompressorFactory::isFormatSupported("zip"));
    EXPECT_TRUE(CompressorFactory::isFormatSupported("ZIP"));
    EXPECT_TRUE(CompressorFactory::isFormatSupported("tar.gz"));
    EXPECT_TRUE(CompressorFactory::isFormatSupported("tar.bz2"));
    EXPECT_TRUE(CompressorFactory::isFormatSupported("7z"));
    
    EXPECT_FALSE(CompressorFactory::isFormatSupported("rar"));
    EXPECT_FALSE(CompressorFactory::isFormatSupported("unknown"));
    EXPECT_FALSE(CompressorFactory::isFormatSupported(""));
    EXPECT_FALSE(CompressorFactory::isFormatSupported("pdf"));
}
