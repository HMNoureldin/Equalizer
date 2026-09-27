/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Binary input and output for signed 16-bit PCM.
 */

#pragma once

#include "IPcmFile.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>

/**
 * @brief Read and write headerless signed 16-bit
 * PCM in the selected byte order.
 * @note No sample-rate or channel metadata is stored.
 * Streams close automatically when the object is destroyed.
 */
class RawPcmSigned16File final : public IPcmFile<std::int16_t>
{
  public:
    /// Byte order of samples in both input and output files.
    enum class ByteOrder {
        LittleEndian, ///< Least significant byte first.
        BigEndian     ///< Most significant byte first.
    };

    /**
     * @brief Create an adapter with both streams closed.
     * @param byteOrder File byte order, independent of host byte order.
     * @note Applies to both reading and writing; raw PCM cannot autodetect it.
     */
    explicit RawPcmSigned16File(ByteOrder byteOrder = ByteOrder::LittleEndian)
        : byteOrder_(byteOrder)
    {
    }

    /**
     * @brief Open input and output after checking file
     * identity.
     * @param inputPath Binary PCM input path.
     * @param outputPath Binary PCM output path; existing
     * data is truncated only after the identity check
     * succeeds.
     * @return Specific opening result. On failure, newly
     * opened streams are closed; AlreadyOpen leaves
     * existing streams intact.
     * @note Detects symbolic and hard links to the input.
     * Paths must not be changed concurrently during this
     * operation.
     */
    [[nodiscard]] OpenResult open(const std::string& inputPath,
                                  const std::string& outputPath) override;

    /**
     * @brief Decode up to maxSamples PCM
     * samples.
     * @param samples Writable buffer for maxSamples
     * samples.
     * @param maxSamples Positive capacity in samples, not
     * bytes.
     * @return Complete sample count and status. EndOfFile
     * can carry samples which must be processed before
     * stopping. Errors may also carry complete samples; the
     * caller decides whether to use them.
     * @note An odd trailing byte reports IncompleteSample.
     * An exact full read reports Success; EOF is detected
     * on the next read. Uses fixed-size scratch storage,
     * with no per-read allocation.
     */
    [[nodiscard]] ReadResult read(std::int16_t* samples,
                                  std::size_t maxSamples) override;

    /**
     * @brief Encode samples in the selected byte order and write
     * them.
     * @param samples Readable buffer of sampleCount
     * samples.
     * @param sampleCount Number of samples, not bytes.
     * @return False for invalid input or a stream write
     * failure.
     * @note Success does not guarantee buffered data is on
     * disk.
     */
    [[nodiscard]] bool write(const std::int16_t* samples,
                             std::size_t sampleCount) override;

    /**
     * @brief Flush buffered output and close its stream.
     * @return True if flushing and closing succeeded with
     * no prior stream errors; false if not open or any
     * write, flush, or close failed.
     * @note Closing is attempted even if flushing fails.
     * Call before reporting success; automatic destruction
     * cannot report final write errors to the caller.
     * Success does not guarantee durability on power loss.
     */
    [[nodiscard]] bool finishOutput() override;

  private:
    /// Selected byte order, shared by reading and writing.
    ByteOrder byteOrder_;
    /// Binary sample input.
    std::ifstream input_;
    /// Buffered binary sample output.
    std::ofstream output_;
};
