/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Argument conversion and gain-range validation.
 */

#include "CommandLine.hpp"

#include <stdexcept>

namespace CommandLine
{
namespace
{
/**
 * @brief Convert a complete numeric argument into a bounded
 * gain.
 * @param value Gain text in decibels.
 * @return Gain in the inclusive range [-12, 12].
 * @throws std::invalid_argument For invalid text or an
 * out-of-range gain.
 * @throws std::out_of_range If numeric conversion overflows
 * or underflows.
 */
double parseGain(const std::string& value)
{
    std::size_t parsed = 0;
    const double gain = std::stod(value, &parsed);
    if (parsed != value.size() ||
        !(gain >= kMinGainDb && gain <= kMaxGainDb)) {
        throw std::invalid_argument(
            "Gain must be between -12 and +12 dB");
    }
    return gain;
}
} // namespace

ProgramOptions parseArguments(int argc, char* argv[])
{
    if (argc != 5) {
        throw std::invalid_argument(
            "Usage: equalizer <input_path> <gain_1kHz> "
            "<gain_2kHz> "
            "<output_path>\n"
            "Gains must be between -12 and +12 dB "
            "(inclusive); 0 = "
            "unchanged.");
    }

    double gain1kHz;
    double gain2kHz;
    try {
        gain1kHz = parseGain(argv[2]);
        gain2kHz = parseGain(argv[3]);
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument(
            "Invalid gain. Use a number from -12 to +12 "
            "dB.");
    } catch (const std::out_of_range&) {
        throw std::invalid_argument(
            "Gain is too large or too small to represent. "
            "Use a number from -12 to +12 dB.");
    }

    return {argv[1], gain1kHz, gain2kHz, argv[4]};
}
} // namespace CommandLine
