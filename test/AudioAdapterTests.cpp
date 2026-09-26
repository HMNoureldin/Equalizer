/**
 * @file
 * @brief Regression tests for PCM conversion and file adapters.
 */
#include "PcmConversion.hpp"
#include "RawPcmFile.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

namespace
{
/// Isolated temporary files for each test; cleanup follows local stream
/// destruction.
class PcmFileTest : public ::testing::Test
{
  protected:
    /// Unique directory owned by this fixture.
    std::filesystem::path directory;
    /// Create an unused directory atomically so parallel tests cannot collide.
    void SetUp() override
    {
        const auto stamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt) {
            auto candidate = std::filesystem::temp_directory_path() /
                             ("equalizer-test-" + std::to_string(stamp) + "-" +
                              std::to_string(attempt));
            if (std::filesystem::create_directory(candidate)) {
                directory = candidate;
                return;
            }
        }
        FAIL() << "Could not create a temporary directory";
    }
    /// Remove only this fixture's temporary directory after each test.
    void TearDown() override
    {
        if (!directory.empty()) {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
            EXPECT_FALSE(error) << error.message();
        }
    }
    /** @brief Build a fixture-local path. @param name File name. @return Full
     * path. */
    std::string path(const std::string& name) const
    {
        return (directory / name).string();
    }
    /** @brief Write known bytes. @param name File name. @param bytes Binary
     * content. */
    void create(const std::string& name, const std::string& bytes)
    {
        std::ofstream file(path(name), std::ios::binary);
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        file.close();
        ASSERT_FALSE(file.fail());
    }
    /** @brief Read raw bytes. @param name File name. @return Binary content. */
    std::string contents(const std::string& name) const
    {
        std::ifstream file(path(name), std::ios::binary);
        return {std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()};
    }
};
} // namespace

/// Every representable PCM16 value must survive conversion to float and back.
TEST(PcmConversionTest, AllPcm16ValuesRoundTrip)
{
    for (int value = -32768; value <= 32767; ++value) {
        const auto sample = static_cast<std::int16_t>(value);
        ASSERT_EQ(
            PcmConversion::floatToPcm16(PcmConversion::pcm16ToFloat(sample)),
            sample);
    }
}

/// Verify saturation, endpoints, scaling, and truncation toward zero.
TEST(PcmConversionTest, ClippingAndScaling)
{
    EXPECT_EQ(PcmConversion::floatToPcm16(2), 32767);
    EXPECT_EQ(PcmConversion::floatToPcm16(-2), -32768);
    EXPECT_EQ(PcmConversion::floatToPcm16(1), 32767);
    EXPECT_EQ(PcmConversion::floatToPcm16(-1), -32768);
    EXPECT_EQ(PcmConversion::floatToPcm16(0.5f), 16384);
    EXPECT_EQ(PcmConversion::floatToPcm16(-0.5f), -16384);
    EXPECT_EQ(PcmConversion::floatToPcm16(1.75f / 32768), 1);
    EXPECT_EQ(PcmConversion::floatToPcm16(-1.75f / 32768), -1);
}

/// Decode and encode independently specified bytes for all signed PCM16 values.
TEST_F(PcmFileTest, LittleEndianEncodingAndDecoding)
{
    std::string bytes;
    for (int value = -32768; value <= 32767; ++value) {
        const unsigned bits = value < 0 ? value + 65536 : value;
        const unsigned char pair[] = {static_cast<unsigned char>(bits & 255),
                                      static_cast<unsigned char>(bits >> 8)};
        bytes.append(reinterpret_cast<const char*>(pair), 2);
    }
    create("in", bytes);
    RawPcmFile file;
    ASSERT_EQ(file.open(path("in"), path("out")),
              RawPcmFile::OpenResult::Success);
    std::vector<std::int16_t> decoded(65537);
    auto result = file.read(decoded.data(), decoded.size());
    ASSERT_EQ(result.status, RawPcmFile::ReadStatus::EndOfFile);
    ASSERT_EQ(result.sampleCount, 65536u);
    for (int i = 0; i < 65536; ++i)
        ASSERT_EQ(decoded[i], i - 32768);
    ASSERT_TRUE(file.write(decoded.data(), result.sampleCount));
    ASSERT_TRUE(file.finishOutput());
    EXPECT_EQ(contents("out"), bytes);
    EXPECT_FALSE(file.finishOutput());
}

/// Verify full, short, empty, and repeated EOF reads without overwriting
/// sentinels.
TEST_F(PcmFileTest, ReadBoundaries)
{
    for (std::size_t count : {0u, 256u, 257u}) {
        SCOPED_TRACE(count);
        create("in", std::string(count * 2, '\0'));
        RawPcmFile file;
        ASSERT_EQ(file.open(path("in"), path("out")),
                  RawPcmFile::OpenResult::Success);
        std::array<std::int16_t, 258> buffer;
        buffer.fill(123);
        auto result = file.read(buffer.data() + 1, 256);
        EXPECT_EQ(result.sampleCount, std::min(count, std::size_t(256)));
        EXPECT_EQ(result.status, count == 0 ? RawPcmFile::ReadStatus::EndOfFile
                                            : RawPcmFile::ReadStatus::Success);
        EXPECT_EQ(buffer.front(), 123);
        EXPECT_EQ(buffer.back(), 123);
        result = file.read(buffer.data() + 1, 256);
        EXPECT_EQ(result.status, RawPcmFile::ReadStatus::EndOfFile);
        EXPECT_EQ(result.sampleCount, count == 257 ? 1u : 0u);
        EXPECT_EQ(file.read(buffer.data(), 1).sampleCount, 0u);
    }
}

/// Odd file lengths must report malformed input, including after an internal
/// chunk.
TEST_F(PcmFileTest, IncompleteSamplesAreReported)
{
    for (std::size_t size : {1u, 3u, 2049u}) {
        create("in", std::string(size, '\0'));
        RawPcmFile file;
        ASSERT_EQ(file.open(path("in"), path("out")),
                  RawPcmFile::OpenResult::Success);
        std::vector<std::int16_t> samples(2048);
        const auto result = file.read(samples.data(), samples.size());
        EXPECT_EQ(result.status, RawPcmFile::ReadStatus::IncompleteSample);
        EXPECT_EQ(result.sampleCount, size / 2);
    }
}

/// Reject invalid calls and recover from failed opens without leaked open
/// state.
TEST_F(PcmFileTest, ErrorsAndRetry)
{
    RawPcmFile file;
    std::int16_t sample = 0;
    EXPECT_EQ(file.read(&sample, 1).status, RawPcmFile::ReadStatus::IoError);
    EXPECT_FALSE(file.write(&sample, 1));
    EXPECT_FALSE(file.finishOutput());
    EXPECT_EQ(file.open(path("missing"), path("out")),
              RawPcmFile::OpenResult::InputOpenFailed);
    EXPECT_FALSE(std::filesystem::exists(path("out")));
    create("in", std::string(2, '\0'));
    EXPECT_EQ(file.open(path("in"), path("missing/out")),
              RawPcmFile::OpenResult::OutputOpenFailed);
    ASSERT_EQ(file.open(path("in"), path("out")),
              RawPcmFile::OpenResult::Success);
    EXPECT_EQ(file.open(path("in"), path("other")),
              RawPcmFile::OpenResult::AlreadyOpen);
    EXPECT_FALSE(std::filesystem::exists(path("other")));
    EXPECT_EQ(file.read(nullptr, 1).status,
              RawPcmFile::ReadStatus::InvalidArgument);
    EXPECT_EQ(file.read(&sample, 0).status,
              RawPcmFile::ReadStatus::InvalidArgument);
    EXPECT_EQ(
        file.read(&sample, std::numeric_limits<std::size_t>::max()).status,
        RawPcmFile::ReadStatus::InvalidArgument);
    EXPECT_FALSE(file.write(nullptr, 1));
    EXPECT_FALSE(file.write(&sample, std::numeric_limits<std::size_t>::max()));
    EXPECT_EQ(file.read(&sample, 1).sampleCount, 1u);
    ASSERT_TRUE(file.write(&sample, 1));
    ASSERT_TRUE(file.finishOutput());
    EXPECT_EQ(contents("out"), std::string(2, '\0'));
}

/// Identical paths and alternate spellings must never truncate the input.
TEST_F(PcmFileTest, SameFilePreservesInput)
{
    create("in", "abcd");
    RawPcmFile file;
    EXPECT_EQ(file.open(path("in"), path("in")),
              RawPcmFile::OpenResult::SameFile);
    EXPECT_EQ(file.open(path("in"), path("./in")),
              RawPcmFile::OpenResult::SameFile);
    EXPECT_EQ(contents("in"), "abcd");
}

/// Hard-link aliases must be recognized where supported by the filesystem.
TEST_F(PcmFileTest, HardLinkPreservesInput)
{
    create("in", "abcd");
    std::error_code error;
    std::filesystem::create_hard_link(path("in"), path("alias"), error);
    if (error)
        GTEST_SKIP() << error.message();
    RawPcmFile file;
    EXPECT_EQ(file.open(path("in"), path("alias")),
              RawPcmFile::OpenResult::SameFile);
    EXPECT_EQ(contents("in"), "abcd");
}

/// Symbolic-link aliases must be recognized where creation is permitted.
TEST_F(PcmFileTest, SymbolicLinkPreservesInput)
{
    create("in", "abcd");
    std::error_code error;
    std::filesystem::create_symlink(path("in"), path("alias"), error);
    if (error)
        GTEST_SKIP() << error.message();
    RawPcmFile file;
    EXPECT_EQ(file.open(path("in"), path("alias")),
              RawPcmFile::OpenResult::SameFile);
    EXPECT_EQ(contents("in"), "abcd");
}

/// A symlink loop yields an identity-check error without consuming input state.
TEST_F(PcmFileTest, IdentityCheckError)
{
    create("in", "abcd");
    std::error_code error;
    std::filesystem::create_symlink("loop", path("loop"), error);
    if (error)
        GTEST_SKIP() << error.message();
    RawPcmFile file;
    EXPECT_EQ(file.open(path("in"), path("loop")),
              RawPcmFile::OpenResult::FileIdentityCheckFailed);
    EXPECT_EQ(file.open(path("in"), path("out")),
              RawPcmFile::OpenResult::Success);
    EXPECT_EQ(contents("in"), "abcd");
}

/// Linux's always-full device verifies that final buffered write failures
/// surface.
TEST_F(PcmFileTest, FinalizationReportsWriteFailure)
{
#ifndef __linux__
    GTEST_SKIP() << "Requires Linux /dev/full";
#else
    if (!std::filesystem::exists("/dev/full"))
        GTEST_SKIP() << "/dev/full unavailable";
    create("in", std::string(2, '\0'));
    RawPcmFile file;
    ASSERT_EQ(file.open(path("in"), "/dev/full"),
              RawPcmFile::OpenResult::Success);
    const std::int16_t sample = 0;
    const bool buffered = file.write(&sample, 1);
    EXPECT_FALSE(file.finishOutput())
        << "write initially succeeded: " << buffered;
#endif
}
