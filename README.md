# Equalizer

**Author:** H.Noureldin  
**Email:** [heshamnoureldin2017@gmail.com](mailto:heshamnoureldin2017@gmail.com)

A C++17 application and standalone library for a two-band audio equalizer.
The default band centers are **1 kHz** and **2 kHz**.

The application reads PCM samples in blocks, converts them to floating point,
applies the equalizer, and converts them back to PCM for output.

## Build and run

Requirements: a C++17 compiler, CMake 3.16+, and GNU Make. Python 3 is needed
only to generate example audio.

From the project root:

```sh
make build BUILD_TYPE=Release
make pcm-1khz
./build/app/equalizer tools/tone_1000hz.pcm 6 0 tools/output_boost.pcm
```

This generates a quiet 1 kHz tone and boosts the first band by 6 dB.

```text
equalizer <input_path> <first_band_gain_db> <second_band_gain_db> <output_path>
```

Both gains default to an allowed range of **-12 to +12 dB**. Positive values
boost, negative values cut, and zero leaves that band's gain neutral.
Input and output must refer to different files.

The CLI uses **raw signed PCM16, little-endian, mono, at 48 kHz** by default.
Files have no header; WAV and MP3 cannot be used directly. Successful processing
preserves the sample count. An incomplete final sample is rejected.

## Generate example audio

| Command | Output | Signal |
| --- | --- | --- |
| `make pcm-1khz` | `tools/tone_1000hz.pcm` | 1 kHz |
| `make pcm-2khz` | `tools/tone_2000hz.pcm` | 2 kHz |
| `make pcm-mixed` | `tools/mixed_tones.pcm` | 250 Hz, 1 kHz, 2 kHz, 6 kHz |

Each file contains five seconds of audio: 240,000 samples, or 480,000 bytes.
The generator uses only Python's standard library and keeps the combined input
amplitude at approximately 10% of full scale to leave boost headroom.
**Running a target again overwrites its output.** Generated PCM files in
`tools/` are ignored by Git.

For a custom filename:

```sh
python3 tools/generate_pcm.py tools/custom.pcm --mode mixed
```

## Configuration

| File | Settings |
| --- | --- |
| `inc/AudioConfig.hpp` | Sample rate, block size, band centers, and Q in `equalizer_app::config` |
| `lib/audioeq/include/audioeq/Config.hpp` | Minimum and maximum gain in `audioeq::config` |

Defaults are **48 kHz, 256 samples per block, Q = 8, centers at 1/2 kHz,
and gains from -12 to +12 dB**. Rebuild the library and application after changing
configuration. The library stays independent of the application config.

Regular DSP tests use the application's settings and configured gain limits.
Test inputs and acceptance tolerances remain independent. If you change band
centers, review the fixed probe frequencies; if you change sample rate, also
update external PCM generation and playback settings.

## Design decisions: why these values?

### Q = 8: reduce interaction between bands

We compared Q values of 2, 4, and 8. For each value, we played a tone at one
band center and applied +12 or -12 dB to the other band. Ideally, the measured
tone would not change.

| Q | Largest unwanted change across the four measurements |
| --- | --- |
| 2 | 1.454 dB |
| 4 | 0.420 dB |
| **8** | **0.109 dB** |

We chose **Q = 8** because it gave the best isolation among the tested values.
Both 4 and 8 satisfy our proposed **0.5 dB** limit, but 8 gives more margin.
That tolerance is a project choice, not a value specified by the assignment.
Higher Q narrows the affected frequency region but can increase ringing and
settling time. These results apply to the default 1 kHz/2 kHz band centers.

### 256 samples: reduce buffering delay

At 48 kHz, smaller blocks represent less buffered audio:

| Block size | One block of audio | Maximum observed DSP time |
| --- | --- | --- |
| **256** | **5.33 ms** | **0.006891 ms** |
| 512 | 10.67 ms | 0.024168 ms |
| 1,024 | 21.33 ms | 0.025981 ms |

We chose **256** because it gave the lowest buffering delay while processing
was comfortably faster than the block interval. Smaller blocks require more
frequent calls. These timings came from one **Debug** exploration run and are
not guaranteed maxima; use Release for representative timing on your machine.

**Full end-to-end latency is still unverified.** The timing check includes one
input block plus observed DSP time. It excludes output buffering, conversion,
filter delay, and device overhead, so it does not prove the full 100 ms target.

### Gain limits and clipping

The **-12 to +12 dB** range is a configurable library/API constraint that bounds
boost and cut. It does **not** prevent clipping; that also depends on the input
level and frequency content. Increasing the limits requires numerical validation.

The equalizer allows floating-point output outside `[-1, +1]`.
`PcmSigned16Conversion::fromFloat()` clips at the PCM output boundary:
values at or above +1 become 32767, and values at or below -1 become -32768.
The conversion requires a non-NaN input. This keeps output representable, but
hard clipping can distort audio. There is no limiter or automatic gain compensation.

### RMS measurements: skip startup settling

Gain tests process the whole signal but exclude the first **0.1 seconds** from
both input and output RMS measurements. This avoids measuring the filter's
startup transient instead of the sustained tone level. It does not discard
application audio or add playback delay. Different Q or frequency settings may
need a different settling interval. The zero-gain test compares every sample.

## Tests and explorations

```sh
make test BUILD_TYPE=Release
make explore BUILD_TYPE=Release
```

`make test` runs GoogleTest through CTest. The current suite has **21 cases**:

| Test file | Checks | Cases |
| --- | --- | --- |
| `EqualizerTests.cpp` | Neutral gain and center-frequency boost/cut | 5 |
| `EqualizerFrequencyResponseTests.cpp` | Band isolation and selected distant tones | 6 |
| `EqualizerEdgeCaseTests.cpp` | Silence, gain validation, empty buffers, reset | 6 |
| `EqualizerLatencyTests.cpp` | Selected block duration plus observed DSP time | 1 |
| `PcmSigned16ConversionTests.cpp` | Clipping and full-scale boundaries | 3 |

File-adapter tests are not currently included. Passing the suite does not prove
behavior at every frequency or every possible configuration.

Explorations print measurements without asserting that a candidate is suitable.
Run just one with `make explore-q` or `make explore-latency BUILD_TYPE=Release`.
Add `GTEST_COLOR=no` for plain-text output. Builds default to Debug unless
`BUILD_TYPE=Release` is supplied.

Tests use an installed GoogleTest package, or download a pinned, hash-verified
GoogleTest v1.14.0 release. The first download requires network access;
application-only builds do not need GoogleTest.

## Reuse the library

`lib/audioeq` contains only the DSP code and its own CMake project. Integrate it
into another C++17 project with:

```cmake
add_subdirectory(path/to/audioeq audioeq-build)
target_link_libraries(my_application PRIVATE audioeq::audioeq)
```

Include `<audioeq/Equalizer.hpp>`. Processing is in-place with fixed filter
state and no per-block allocation. Invalid construction parameters throw
exceptions, so the current API requires exception support. Calls modifying the
same instance must not run concurrently with processing.

To build only the library:

```sh
cmake -S lib/audioeq -B build/core -DCMAKE_BUILD_TYPE=Release
cmake --build build/core
```

## Project organization

See the [sequence diagram](docs/images/sequence-diagram.png) for the processing
flow and the [class diagram](docs/images/class-diagram.png) for component
relationships.

| Location | Responsibility |
| --- | --- |
| `lib/audioeq/` | Standalone Equalizer and Biquad DSP library |
| `inc/`, `src/` | Application, CLI, file adapters, conversion, logging |
| `test/` | Tests, measurement helpers, and explorations |
| `tools/` | Python PCM generator and generated audio |

`IPcmFile<SampleType>` defines the typed file interface. The only implemented
format is `RawPcmSigned16File`, with selectable byte order for both reading
and writing; the CLI defaults to little-endian. To select big-endian in C++:

```cpp
RawPcmSigned16File file(RawPcmSigned16File::ByteOrder::BigEndian);
```

Byte order is not automatically detected. The adapter handles file bytes;
`PcmSigned16Conversion::toFloat()` and `fromFloat()` handle numeric samples.
Other PCM formats require their own adapters and conversions.

## Formatting and documentation

```sh
make format        # Requires clang-format 18
make format-check
make docs          # Requires Doxygen
make show-docs     # Generate and open HTML documentation on Linux
```

Both documentation commands delete old generated documentation before rebuilding.
Doxygen output is in `build/docs/html/index.html`. Use `make help` for available
commands. `make clean` removes `build/app`, `build/tests`, and `build/docs`;
it preserves generated PCM files in `tools/` and standalone builds in `build/core`.
