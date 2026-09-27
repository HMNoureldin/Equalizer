/**
 * @file
 * @brief Check PCM16 decoding and encoding against explicit byte sequences.
 */
#include "RawPcm16File.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include <gtest/gtest.h>

namespace
{
/// Temporary files owned by one PCM file test.
class RawPcm16FileTest : public ::testing::Test
{
  protected:
    /// Unique directory containing the test's input and output.
    std::filesystem::path directory;

    /// Create an isolated directory without replacing an existing one.
    void SetUp() override
    {
        const auto stamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt) {
            directory = std::filesystem::temp_directory_path() /
                        ("audioeq-pcm-" + std::to_string(stamp) + "-" +
                         std::to_string(attempt));
            if (std::filesystem::create_directory(directory))
                return;
        }
        directory.clear();
        FAIL() << "Unable to create temporary directory";
    }

    /// Remove test files after all streams have been destroyed.
    void TearDown() override
    {
        if (!directory.empty()) {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }
    }

    /**
     * @brief Verify read and write independently against known PCM bytes.
     * @param file Adapter whose selected byte order is under test.
     * @param expectedBytes Explicit representation of the known samples.
     */
    void checkEncoding(RawPcm16File& file,
                       const std::vector<unsigned char>& expectedBytes)
    {
        const auto input = directory / "input.pcm";
        const auto output = directory / "output.pcm";
        {
            std::ofstream stream(input, std::ios::binary);
            stream.write(reinterpret_cast<const char*>(expectedBytes.data()),
                         static_cast<std::streamsize>(expectedBytes.size()));
            stream.close();
            ASSERT_TRUE(stream.good());
        }
        ASSERT_EQ(file.open(input.string(), output.string()),
                  RawPcm16File::OpenResult::Success);
        const std::array<std::int16_t, 5> expectedSamples{0, 0x1234, -2, 32767,
                                                          -32768};
        std::array<std::int16_t, 6> decoded{};
        const auto result = file.read(decoded.data(), decoded.size());
        ASSERT_EQ(result.status, RawPcm16File::ReadStatus::EndOfFile);
        ASSERT_EQ(result.sampleCount, expectedSamples.size());
        for (std::size_t i = 0; i < expectedSamples.size(); ++i)
            EXPECT_EQ(decoded[i], expectedSamples[i]) << "Sample " << i;

        ASSERT_TRUE(file.write(expectedSamples.data(), expectedSamples.size()));
        ASSERT_TRUE(file.finishOutput());
        std::ifstream stream(output, std::ios::binary);
        ASSERT_TRUE(stream.is_open());
        const std::vector<unsigned char> actualBytes{
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>()};
        EXPECT_EQ(actualBytes, expectedBytes);
    }
};

/// Preserve the application's existing little-endian default.
TEST_F(RawPcm16FileTest, DefaultsToLittleEndian)
{
    RawPcm16File file;
    checkEncoding(file, {0, 0, 0x34, 0x12, 0xfe, 0xff, 0xff, 0x7f, 0, 0x80});
}

/// Explicit little-endian selection handles positive and negative samples.
TEST_F(RawPcm16FileTest, SupportsLittleEndian)
{
    RawPcm16File file(RawPcm16File::ByteOrder::LittleEndian);
    checkEncoding(file, {0, 0, 0x34, 0x12, 0xfe, 0xff, 0xff, 0x7f, 0, 0x80});
}

/// Big-endian selection handles both signed endpoints and byte order.
TEST_F(RawPcm16FileTest, SupportsBigEndian)
{
    RawPcm16File file(RawPcm16File::ByteOrder::BigEndian);
    checkEncoding(file, {0, 0, 0x12, 0x34, 0xff, 0xfe, 0x7f, 0xff, 0x80, 0});
}
} // namespace
