/**
 * @file
 * @brief Typed PCM file interface and format-independent operation results.
 */
#pragma once

#include <cstddef>
#include <string>

/// Results shared by PCM file implementations of all sample types.
namespace pcm
{
/// Result of opening the input/output pair.
enum class OpenResult {
    Success,                 ///< Both streams opened.
    AlreadyOpen,             ///< An existing stream is open.
    InputOpenFailed,         ///< Input could not be opened.
    FileIdentityCheckFailed, ///< File identity could
                             ///< not be checked.
    SameFile,                ///< Paths identify the same file.
    OutputOpenFailed         ///< Output could not be opened.
};

/// Outcome of a read, independent of the complete
/// sample count.
enum class ReadStatus {
    Success,         ///< Requested samples were read.
    EndOfFile,       ///< Clean EOF; may include a short final
                     ///< block.
    InvalidArgument, ///< Invalid buffer or sample
                     ///< count.
    IoError,         ///< Input is closed or a read failed.
    IncompleteSample ///< EOF partway through an encoded sample.
};

/// Complete decoded samples and the reason reading
/// stopped.
struct ReadResult
{
    std::size_t sampleCount; ///< Complete samples
                             ///< placed in the buffer.
    ReadStatus status;       ///< Read outcome.
};

} // namespace pcm

/**
 * @brief Interface for file input/output of decoded PCM samples.
 * @tparam SampleType Numeric type used in caller sample buffers.
 * @note The concrete adapter defines file encoding and byte order. Different
 * sample types form different interfaces; no implicit sample conversion occurs.
 */
template <typename SampleType> class IPcmFile
{
  public:
    /// Shared opening outcome, independent of sample type.
    using OpenResult = pcm::OpenResult;
    /// Shared read outcome, independent of sample type.
    using ReadStatus = pcm::ReadStatus;
    /// Shared complete-sample count and read status.
    using ReadResult = pcm::ReadResult;

    /// Allow safe cleanup through an interface pointer.
    virtual ~IPcmFile() = default;

    /**
     * @brief Open a distinct input/output file pair.
     * @param inputPath Input file path.
     * @param outputPath Output file path.
     * @return Opening outcome; a failed open must not retain newly opened
     * streams.
     * @note An AlreadyOpen result leaves existing streams intact.
     */
    [[nodiscard]] virtual OpenResult open(const std::string& inputPath,
                                          const std::string& outputPath) = 0;

    /**
     * @brief Read and decode samples into caller-owned storage.
     * @param samples Writable buffer; must not be null.
     * @param maxSamples Positive buffer capacity in samples, not bytes.
     * @return Complete sample count and status; EOF or errors may carry
     * samples.
     */
    [[nodiscard]] virtual ReadResult read(SampleType* samples,
                                          std::size_t maxSamples) = 0;

    /**
     * @brief Encode and write samples from caller-owned storage.
     * @param samples Readable sample buffer; must not be null.
     * @param sampleCount Number of samples to write, not bytes.
     * @return False on invalid input or write failure.
     */
    [[nodiscard]] virtual bool write(const SampleType* samples,
                                     std::size_t sampleCount) = 0;

    /**
     * @brief Flush and close output, reporting final write errors.
     * @return True if output finalized successfully; false on failure or if
     * closed.
     * @note Destruction alone cannot report buffered write failures.
     */
    [[nodiscard]] virtual bool finishOutput() = 0;
};
