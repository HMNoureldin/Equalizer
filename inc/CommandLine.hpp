/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Command-line argument parsing.
 */

#pragma once

#include <string>

/** @brief Command-line argument parsing and validation.
 */
namespace CommandLine
{
/** @brief Validated arguments; paths are stored without
 * checking file access. */
struct ProgramOptions
{
    /// Raw PCM input path.
    std::string inputPath;
    /// Gain at 1 kHz in the core-supported dB range; zero
    /// means unchanged.
    double gain1kHz;
    /// Gain at 2 kHz in the core-supported dB range; zero
    /// means unchanged.
    double gain2kHz;
    /// Intended processed PCM output path.
    std::string outputPath;
};

/**
 * @brief Parse the four positional application arguments.
 * @param argc Argument count, including the executable
 * name.
 * @param argv Argument strings in standard main() layout.
 * @return Paths and validated gains in decibels.
 * @throws std::invalid_argument If the count or either gain
 * is invalid.
 * @pre argv contains argc valid, null-terminated strings.
 * @note Does not open files or process audio.
 */
ProgramOptions parseArguments(int argc, char* argv[]);

} // namespace CommandLine
