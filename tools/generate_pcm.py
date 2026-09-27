#!/usr/bin/env python3
"""Generate five seconds of quiet single or mixed tones as raw PCM16 little-endian.

The output is mono at 48 kHz with no header, matching the equalizer CLI.
Run from the project root: python3 tools/generate_pcm.py --mode mixed
"""

import argparse
import math
from pathlib import Path
import struct


SAMPLE_RATE_HZ = 48000
DURATION_SECONDS = 5
TONES_HZ = {
    "1khz": (1000,),
    "2khz": (2000,),
    "mixed": (250, 1000, 2000, 6000),
}
OUTPUT_NAMES = {
    "1khz": "tone_1000hz.pcm",
    "2khz": "tone_2000hz.pcm",
    "mixed": "mixed_tones.pcm",
}
AMPLITUDE = 0.1  # Leave headroom for the equalizer's gain boosts.


def main():
    """Write the test tone to the requested output path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "output", nargs="?", type=Path,
        help="optional output path (default: mode-specific file beside this script)",
    )
    parser.add_argument("--mode", choices=TONES_HZ, default="1khz",
                        help="signal to generate (default: 1khz)")
    args = parser.parse_args()
    if args.output is None:
        args.output = Path(__file__).resolve().parent / OUTPUT_NAMES[args.mode]
    frequencies = TONES_HZ[args.mode]
    amplitude_per_tone = AMPLITUDE / len(frequencies)
    sample_count = SAMPLE_RATE_HZ * DURATION_SECONDS

    try:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        # Replace any existing contents when regenerating the selected signal.
        with args.output.open("wb") as output:
            for index in range(sample_count):
                value = sum(
                    amplitude_per_tone
                    * math.sin(2 * math.pi * frequency * index / SAMPLE_RATE_HZ)
                    for frequency in frequencies
                )
                sample = round(value * 32767)
                output.write(struct.pack("<h", sample))
    except OSError as error:
        parser.exit(1, f"Cannot create {args.output}: {error}\n")

    print(f"Created {args.output}")
    print("Tone frequencies (Hz): " + ", ".join(map(str, frequencies)))
    print(f"{DURATION_SECONDS} seconds; "
          f"{SAMPLE_RATE_HZ} Hz sample rate; mono; signed PCM16 little-endian")
    print(f"{sample_count} samples; {sample_count * 2} bytes; no header")


if __name__ == "__main__":
    main()
