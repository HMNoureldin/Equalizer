/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Process mono 48 kHz signed 16-bit PCM audio.
 */

#include "CommandLine.hpp"
#include "Equalizer.hpp"
#include "Logger.hpp"
#include "PcmConversion.hpp"
#include "RawPcmFile.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>

namespace
{
/// Required input sample rate in Hz.
constexpr float kSampleRateHz = 48000.0f;
/// Samples processed per block.
constexpr std::size_t kBlockSize = 1024;
} // namespace

/**
 * @brief Parse arguments and process the input PCM file.
 * @param argc Argument count, including the executable
 * name.
 * @param argv Executable name followed by input, two gains,
 * and output.
 * @return Zero on success, or one on failure.
 * @note Input must be mono 48 kHz signed 16-bit PCM
 * in little-endian byte order. Input and output must
 * identify different files. Output is explicitly finalized
 * before success is reported.
 */
int main(int argc, char* argv[])
{
    Logger::setLevel(Logger::Level::Debug);
    LOG_INFO("Starting Equalizer...");

    try {

        // -------------------------------------------------
        // 1. Command-line parsing
        // -------------------------------------------------
        const auto options = CommandLine::parseArguments(argc, argv);

        const std::string inputPath = options.inputPath;
        const double gain1kHz = options.gain1kHz;
        const double gain2kHz = options.gain2kHz;
        const std::string outputPath = options.outputPath;

        LOG_DEBUG("Input: " + inputPath);
        LOG_DEBUG("Gain at 1 kHz: " + std::to_string(gain1kHz) + " dB");
        LOG_DEBUG("Gain at 2 kHz: " + std::to_string(gain2kHz) + " dB");
        LOG_DEBUG("Output: " + outputPath);
        LOG_DEBUG("Arguments parsed successfully");

        // -------------------------------------------------
        // 2. Validate and configure DSP
        // -------------------------------------------------

        // Validate settings before file opening can
        // truncate output.
        audioeq::Equalizer equalizer(
            kSampleRateHz, {1000.0f, 2.0f, static_cast<float>(gain1kHz)},
            {2000.0f, 2.0f, static_cast<float>(gain2kHz)});

        // -------------------------------------------------
        // 3. Open files
        // -------------------------------------------------
        RawPcmFile pcmFile;

        const auto openResult = pcmFile.open(inputPath, outputPath);
        switch (openResult) {
        case RawPcmFile::OpenResult::Success:
            break;
        case RawPcmFile::OpenResult::AlreadyOpen:
            LOG_ERROR("Files are already open");
            return 1;
        case RawPcmFile::OpenResult::InputOpenFailed:
            LOG_ERROR("Failed to open input file: " + inputPath);
            return 1;
        case RawPcmFile::OpenResult::FileIdentityCheckFailed:
            LOG_ERROR("Could not verify that input and "
                      "output differ");
            return 1;
        case RawPcmFile::OpenResult::SameFile:
            LOG_ERROR("Input and output refer to the same file");
            return 1;
        case RawPcmFile::OpenResult::OutputOpenFailed:
            LOG_ERROR("Failed to open output file: " + outputPath);
            return 1;
        }

        // -------------------------------------------------
        // 4. Allocate buffers ONCE
        // -------------------------------------------------

        std::array<std::int16_t, kBlockSize> pcmBuffer{};
        std::array<float, kBlockSize> processingBuffer{};

        // -------------------------------------------------
        // 5. Streaming loop
        // -------------------------------------------------

        while (true) {
            const auto result =
                pcmFile.read(pcmBuffer.data(), pcmBuffer.size());
            switch (result.status) {
            case RawPcmFile::ReadStatus::Success:
            case RawPcmFile::ReadStatus::EndOfFile:
                break;
            case RawPcmFile::ReadStatus::InvalidArgument:
                LOG_ERROR("Invalid PCM read buffer or size");
                return 1;
            case RawPcmFile::ReadStatus::IoError:
                LOG_ERROR("Failed to read input file");
                return 1;
            case RawPcmFile::ReadStatus::IncompleteSample:
                LOG_ERROR("Input ends with an incomplete "
                          "16-bit sample");
                return 1;
            }
            const auto samplesRead = result.sampleCount;
            if (samplesRead == 0) {
                break;
            }

            // PCM -> float
            for (std::size_t i = 0; i < samplesRead; ++i) {
                processingBuffer[i] = PcmConversion::pcm16ToFloat(pcmBuffer[i]);
            }

            // DSP
            equalizer.process(processingBuffer.data(), samplesRead);

            // float -> PCM
            for (std::size_t i = 0; i < samplesRead; ++i) {
                pcmBuffer[i] = PcmConversion::floatToPcm16(processingBuffer[i]);
            }

            // Write processed block
            if (!pcmFile.write(pcmBuffer.data(), samplesRead)) {
                LOG_ERROR("Failed to write output file");
                return 1;
            }
            if (result.status == RawPcmFile::ReadStatus::EndOfFile) {
                break;
            }
        }

        if (!pcmFile.finishOutput()) {
            LOG_ERROR("Failed to finish writing output file");
            return 1;
        }

    } catch (const std::exception& error) {
        LOG_ERROR(error.what());
        return 1;
    }
    LOG_INFO("Equalizer finished processing");

    return 0;
}
