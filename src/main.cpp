/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Process mono 48 kHz signed 16-bit PCM audio.
 */

#include "AudioConfig.hpp"
#include "CommandLine.hpp"
#include "Logger.hpp"
#include "PcmConversion.hpp"
#include "RawPcm16File.hpp"
#include <audioeq/Equalizer.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>

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
        const float firstBandGainDb = options.firstBandGainDb;
        const float secondBandGainDb = options.secondBandGainDb;
        const std::string outputPath = options.outputPath;

        LOG_DEBUG("Input: " + inputPath);
        LOG_DEBUG("First band (" +
                  std::to_string(equalizer_app::config::kFirstBandFrequencyHz) +
                  " Hz) gain: " + std::to_string(firstBandGainDb) + " dB");
        LOG_DEBUG(
            "Second band (" +
            std::to_string(equalizer_app::config::kSecondBandFrequencyHz) +
            " Hz) gain: " + std::to_string(secondBandGainDb) + " dB");
        LOG_DEBUG("Output: " + outputPath);
        LOG_DEBUG("Arguments parsed successfully");

        // -------------------------------------------------
        // 2. Validate and configure DSP
        // -------------------------------------------------

        // Validate settings before file opening can
        // truncate output.
        audioeq::Equalizer equalizer(
            equalizer_app::config::kSampleRateHz,
            {equalizer_app::config::kFirstBandFrequencyHz,
             equalizer_app::config::kSelectedQ, firstBandGainDb},
            {equalizer_app::config::kSecondBandFrequencyHz,
             equalizer_app::config::kSelectedQ, secondBandGainDb});

        // -------------------------------------------------
        // 3. Open files
        // -------------------------------------------------
        RawPcm16File pcmFile;

        const auto openResult = pcmFile.open(inputPath, outputPath);
        switch (openResult) {
            case RawPcm16File::OpenResult::Success:
                break;
            case RawPcm16File::OpenResult::AlreadyOpen:
                LOG_ERROR("Files are already open");
                return 1;
            case RawPcm16File::OpenResult::InputOpenFailed:
                LOG_ERROR("Failed to open input file: " + inputPath);
                return 1;
            case RawPcm16File::OpenResult::FileIdentityCheckFailed:
                LOG_ERROR("Could not verify that input and "
                          "output differ");
                return 1;
            case RawPcm16File::OpenResult::SameFile:
                LOG_ERROR("Input and output refer to the same file");
                return 1;
            case RawPcm16File::OpenResult::OutputOpenFailed:
                LOG_ERROR("Failed to open output file: " + outputPath);
                return 1;
        }

        // -------------------------------------------------
        // 4. Allocate buffers ONCE
        // -------------------------------------------------

        std::array<std::int16_t, equalizer_app::config::kProcessingBlockSize>
            pcmBuffer{};
        std::array<float, equalizer_app::config::kProcessingBlockSize>
            processingBuffer{};

        // -------------------------------------------------
        // 5. Streaming loop
        // -------------------------------------------------

        while (true) {
            const auto result =
                pcmFile.read(pcmBuffer.data(), pcmBuffer.size());
            switch (result.status) {
                case RawPcm16File::ReadStatus::Success:
                case RawPcm16File::ReadStatus::EndOfFile:
                    break;
                case RawPcm16File::ReadStatus::InvalidArgument:
                    LOG_ERROR("Invalid PCM read buffer or size");
                    return 1;
                case RawPcm16File::ReadStatus::IoError:
                    LOG_ERROR("Failed to read input file");
                    return 1;
                case RawPcm16File::ReadStatus::IncompleteSample:
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
            if (result.status == RawPcm16File::ReadStatus::EndOfFile) {
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
