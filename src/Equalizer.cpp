/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Cascaded peaking filters with bounded band gains.
 */

#include "Equalizer.hpp"

#include <cmath>
#include <stdexcept>

namespace audioeq
{
Equalizer::Equalizer(float sampleRateHz, BandConfig firstBand,
                     BandConfig secondBand)
    : sampleRateHz_(sampleRateHz), firstBand_(firstBand),
      secondBand_(secondBand)
{
    if (!changeBandGainDb(Band::First, firstBand_.gainDb) ||
        !changeBandGainDb(Band::Second, secondBand_.gainDb)) {
        throw std::invalid_argument("Band gains must be finite and within the "
                                    "supported gain limits.");
    }
}

bool Equalizer::changeBandGainDb(Band band, float gainDb)
{
    if (!std::isfinite(gainDb) || gainDb < kMinGainDb || gainDb > kMaxGainDb) {
        return false;
    }

    switch (band) {
    case Band::First:
        firstFilter_.configurePeaking(sampleRateHz_, firstBand_.frequencyHz,
                                      firstBand_.q, gainDb);
        firstBand_.gainDb = gainDb;
        return true;
    case Band::Second:
        secondFilter_.configurePeaking(sampleRateHz_, secondBand_.frequencyHz,
                                       secondBand_.q, gainDb);
        secondBand_.gainDb = gainDb;
        return true;
    }
    return false;
}

void Equalizer::process(float* samples, std::size_t sampleCount) noexcept
{
    if (samples == nullptr) {
        return;
    }

    for (std::size_t i = 0; i < sampleCount; ++i) {
        float sample = samples[i];

        sample = firstFilter_.processSample(sample);
        sample = secondFilter_.processSample(sample);

        samples[i] = sample;
    }
}

void Equalizer::reset() noexcept
{
    firstFilter_.reset();
    secondFilter_.reset();
}
} // namespace audioeq
