/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Command-line argument validation.
 */

#include "CommandLine.hpp"
#include "AudioConfig.hpp"
#include <audioeq/Equalizer.hpp>

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
float parseGain(const std::string& value)
{
    std::size_t parsed = 0;
    const float gain = std::stof(value, &parsed);
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
        throw std::invalid_argument(
            "Usage: equalizer <input_path> <gain_first_band> "
            "<gain_second_band> "
            "<output_path>\n"
            "Gains must be within " +
            gainRangeDescription() + "; 0 = unchanged.\nFirst band: " +
            std::to_string(equalizer_app::config::kFirstBandFrequencyHz) +
            " Hz; second band: " +
            std::to_string(equalizer_app::config::kSecondBandFrequencyHz) +
            " Hz.");
    }

    float firstBandGainDb;
    float secondBandGainDb;
    try {
        firstBandGainDb = parseGain(argv[2]);
        secondBandGainDb = parseGain(argv[3]);
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Invalid gain. Use a number from " +
                                    gainRangeDescription() + ".");
    } catch (const std::out_of_range&) {
        throw std::invalid_argument(
            "Gain is too large or too small to represent. "
            "Use a number from " +
            gainRangeDescription() + ".");
    }

    return {argv[1], firstBandGainDb, secondBandGainDb, argv[4]};
}
} // namespace CommandLine
