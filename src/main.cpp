/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Application entry point; validates options and
 * logs settings.
 */

#include "CommandLine.hpp"
#include "Logger.hpp"

#include <exception>
#include <string>

/**
 * @brief Enable debug logging and validate application
 * arguments.
 * @param argc Argument count, including the executable
 * name.
 * @param argv Executable name followed by input, two gains,
 * and output.
 * @return Zero on success, or one if argument parsing
 * fails.
 * @note Audio processing is not implemented; no audio files
 * are opened.
 */
int main(int argc, char* argv[])
{
    Logger::setLevel(Logger::Level::Debug);
    LOG_INFO("Starting Equalizer...");

    try {
        const auto options =
            CommandLine::parseArguments(argc, argv);

        LOG_DEBUG("Input: " + options.inputPath);
        LOG_DEBUG("Gain at 1 kHz: " +
                  std::to_string(options.gain1kHz) + " dB");
        LOG_DEBUG("Gain at 2 kHz: " +
                  std::to_string(options.gain2kHz) + " dB");
        LOG_DEBUG("Output: " + options.outputPath);
        LOG_DEBUG("Arguments parsed successfully");
    } catch (const std::exception& error) {
        LOG_ERROR(error.what());
        return 1;
    }
    LOG_INFO("Equalizer finished processing");

    return 0;
}
