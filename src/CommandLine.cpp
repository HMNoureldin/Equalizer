/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Command-line argument validation.
 */

#include "CommandLine.hpp"
#include "Equalizer.hpp"

#include <sstream>
#include <stdexcept>

namespace CommandLine
{
namespace
{
/**
 * @brief Format the core's supported gain range for CLI
 * messages.
 * @return Inclusive range in decibels.
 */
std::string gainRangeDescription()
{
    std::ostringstream text;
    text << audioeq::Equalizer::kMinGainDb << " to "
         << audioeq::Equalizer::kMaxGainDb << " dB (inclusive)";
    return text.str();
}

/**
 * @brief Convert a complete numeric argument into a bounded
 * gain.
 * @param value Gain text in decibels.
 * @return Gain within the core's inclusive gain limits.
 * @throws std::invalid_argument For invalid text or an
 * out-of-range gain.
 * @throws std::out_of_range If numeric conversion overflows
 * or underflows.
 */
double parseGain(const std::string& value)
{
    std::size_t parsed = 0;
    const double gain = std::stod(value, &parsed);
    if (parsed != value.size() || !(gain >= audioeq::Equalizer::kMinGainDb &&
                                    gain <= audioeq::Equalizer::kMaxGainDb)) {
        throw std::invalid_argument("Gain must be within " +
                                    gainRangeDescription());
    }
    return gain;
}
} // namespace

ProgramOptions parseArguments(int argc, char* argv[])
{
    if (argc != 5) {
        throw std::invalid_argument("Usage: equalizer <input_path> <gain_1kHz> "
                                    "<gain_2kHz> "
                                    "<output_path>\n"
                                    "Gains must be within " +
                                    gainRangeDescription() +
                                    "; 0 = unchanged.");
    }

    double gain1kHz;
    double gain2kHz;
    try {
        gain1kHz = parseGain(argv[2]);
        gain2kHz = parseGain(argv[3]);
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Invalid gain. Use a number from " +
                                    gainRangeDescription() + ".");
    } catch (const std::out_of_range&) {
        throw std::invalid_argument(
            "Gain is too large or too small to represent. "
            "Use a number from " +
            gainRangeDescription() + ".");
    }

    return {argv[1], gain1kHz, gain2kHz, argv[4]};
}
} // namespace CommandLine
