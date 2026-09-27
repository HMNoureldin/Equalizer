/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Stream-based signed 16-bit PCM file handling.
 */

#include "RawPcm16File.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <filesystem>
#include <limits>
#include <system_error>

namespace
{
static_assert(CHAR_BIT == 8, "PCM files require 8-bit bytes");
/// Fixed conversion buffer: 1024 samples, independent of
/// caller block size.
constexpr std::size_t kPcmBytes = 2048;

/**
 * @brief Check that a sample count fits a stream byte
 * count.
 * @param count Number of 16-bit samples.
 * @return True if multiplication and conversion are safe.
 */
bool validSampleCount(std::size_t count)
{
    const auto maxBytes = static_cast<std::uintmax_t>(
        std::numeric_limits<std::streamsize>::max());
    return count <= maxBytes / sizeof(std::int16_t) &&
           count <=
               std::numeric_limits<std::size_t>::max() / sizeof(std::int16_t);
}
} // namespace

RawPcm16File::OpenResult RawPcm16File::open(const std::string& inputPath,
                                            const std::string& outputPath)
{
    // 1. Preserve any files already owned by this object.
    if (input_.is_open() || output_.is_open()) {
        return OpenResult::AlreadyOpen;
    }

    // 2. Open input locally until the whole operation
    // succeeds. Any early return destroys this stream and
    // closes the file automatically (RAII), including an
    // output-open failure.
    std::ifstream input(inputPath, std::ios::binary);
    if (!input.is_open()) {
        return OpenResult::InputOpenFailed;
    }

    // 3. Check identity before output opening can erase
    // data. Existing paths are compared by file identity,
    // so symbolic and hard links are detected too. A new
    // output may be created.
    std::error_code error;
    const bool outputExists = std::filesystem::exists(outputPath, error);
    if (error) {
        return OpenResult::FileIdentityCheckFailed;
    }
    if (outputExists) {
        const bool sameFile =
            std::filesystem::equivalent(inputPath, outputPath, error);
        if (error) {
            return OpenResult::FileIdentityCheckFailed;
        }
        if (sameFile) {
            return OpenResult::SameFile;
        }
    }

    // 4. Open output only after validation; existing data
    // is truncated.
    std::ofstream output(outputPath, std::ios::binary);
    if (!output.is_open()) {
        return OpenResult::OutputOpenFailed;
    }

    // 5. Transfer both open streams to the object on
    // success. The local streams receive the previously
    // closed member streams, so their destruction leaves
    // the newly opened files available.
    input_.swap(input);
    output_.swap(output);
    return OpenResult::Success;
}

RawPcm16File::ReadResult RawPcm16File::read(std::int16_t* samples,
                                            std::size_t maxSamples)
{
    if (samples == nullptr || maxSamples == 0 ||
        !validSampleCount(maxSamples)) {
        return {0, ReadStatus::InvalidArgument};
    }
    if (!input_.is_open()) {
        return {0, ReadStatus::IoError};
    }

    const std::size_t lowByteOffset =
        byteOrder_ == ByteOrder::LittleEndian ? 0 : 1;
    const std::size_t highByteOffset = 1 - lowByteOffset;
    std::array<unsigned char, kPcmBytes> bytes;
    std::size_t total = 0;
    while (total < maxSamples) {
        const auto count = std::min(maxSamples - total, bytes.size() / 2);
        input_.read(reinterpret_cast<char*>(bytes.data()),
                    static_cast<std::streamsize>(count * 2));
        const auto bytesRead = static_cast<std::size_t>(input_.gcount());
        for (std::size_t i = 0; i < bytesRead / 2; ++i) {
            const std::int32_t value =
                static_cast<std::int32_t>(bytes[2 * i + lowByteOffset]) |
                (static_cast<std::int32_t>(bytes[2 * i + highByteOffset]) << 8);
            // Convert two's-complement bits without
            // out-of-range signed casts.
            samples[total + i] = static_cast<std::int16_t>(
                value >= 32768 ? value - 65536 : value);
        }
        total += bytesRead / 2;
        if (input_.bad() || (input_.fail() && !input_.eof())) {
            return {total, ReadStatus::IoError};
        }
        if (bytesRead % 2 != 0) {
            return {total, ReadStatus::IncompleteSample};
        }
        if (input_.eof()) {
            return {total, ReadStatus::EndOfFile};
        }
    }
    return {total, ReadStatus::Success};
}

bool RawPcm16File::write(const std::int16_t* samples, std::size_t sampleCount)
{
    if (!output_.is_open() || samples == nullptr ||
        !validSampleCount(sampleCount)) {
        return false;
    }

    const std::size_t lowByteOffset =
        byteOrder_ == ByteOrder::LittleEndian ? 0 : 1;
    const std::size_t highByteOffset = 1 - lowByteOffset;
    std::array<unsigned char, kPcmBytes> bytes;
    std::size_t total = 0;
    while (total < sampleCount) {
        const auto count = std::min(sampleCount - total, bytes.size() / 2);
        for (std::size_t i = 0; i < count; ++i) {
            const auto value = static_cast<std::uint16_t>(samples[total + i]);
            bytes[2 * i + lowByteOffset] =
                static_cast<unsigned char>(value & 0xff);
            bytes[2 * i + highByteOffset] =
                static_cast<unsigned char>(value >> 8);
        }
        output_.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(count * 2));
        if (!output_.good()) {
            return false;
        }
        total += count;
    }
    return output_.good();
}

bool RawPcm16File::finishOutput()
{
    if (!output_.is_open()) {
        return false;
    }

    output_.flush();
    const bool flushed = output_.good();
    output_.close();

    return flushed && !output_.fail();
}
