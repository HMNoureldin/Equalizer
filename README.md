# Equalizer

**Author:** H.Noureldin  
**Email:** [heshamnoureldin2017@gmail.com](mailto:heshamnoureldin2017@gmail.com)

A C++17 command-line application for applying gain at **1 kHz** and **2 kHz** to raw PCM audio.

> **Development status:** Streaming PCM processing, configurable two-band DSP, file output, and automated tests are implemented. The complete end-to-end latency assessment remains outstanding.

## Standalone DSP library

`lib/audioeq` contains only `Equalizer` and `Biquad`, with its own CMake project.
It requires C++17 and has no dependency on the CLI, file handling, logging,
GoogleTest, or application settings. Copy this directory into another project
and integrate it with:

```cmake
add_subdirectory(path/to/audioeq audioeq-build)
target_link_libraries(my_application PRIVATE audioeq::audioeq)
```

Public headers use library-qualified paths:

```cpp
#include <audioeq/Equalizer.hpp>
```

The target supplies its include directory and C++17 requirement to consumers.
To build the library alone, without configuring the application or tests:

```sh
cmake -S lib/audioeq -B build/core -DCMAKE_BUILD_TYPE=Release
cmake --build build/core
```

Application defaults (Q and block size) remain in `inc/AudioConfig.hpp`.
Processing remains in-place and allocation-free. Invalid construction parameters
throw exceptions, so embedded integrations must support C++ exceptions with the
current API. Packaging the library separately does not change that requirement.

## Audio format

The `RawPcmSigned16File` adapter reads and writes headerless signed 16-bit PCM
in either little-endian or big-endian byte order. Little-endian is the default.
The command-line application continues to use little-endian, mono PCM at 48 kHz. Byte order is encoded explicitly, independently of the host.
A final short block of complete samples is processed normally. An odd
file length (a trailing incomplete sample) is rejected with an error;
an error can leave a partial output file.

### Typed PCM file interface

`IPcmFile<SampleType>` declares opening, reading, writing, and checked output
finalization. Operation results live in `pcm` and are shared across sample types.
`RawPcmSigned16File` implements `IPcmFile<std::int16_t>`; it is the only concrete
format currently supported. Future unsigned or floating-point adapters can
implement their own typed interfaces and encoding rules.

```cpp
RawPcmSigned16File adapter;
IPcmFile<std::int16_t>& file = adapter;
```

The application calls the adapter through this interface.
This requires no heap allocation. Virtual dispatch occurs per file operation,
not per sample. The interface has a virtual destructor for safe cleanup.
Adding another format also requires appropriate sample conversion in its caller;
an interface alone does not add unsigned or floating-point file support.

### Selecting file byte order in C++

```cpp
RawPcmSigned16File littleEndianFile; // Default, used by the current CLI.
RawPcmSigned16File bigEndianFile(RawPcmSigned16File::ByteOrder::BigEndian);
```

The constructor option applies to both input and output for that object. It
is independent of the computer's byte order. Raw PCM contains no header, so
byte order must be known by the caller; it is not detected automatically.
The CLI and Python generator retain their little-endian format. No CLI
byte-order flag or separate input/output byte orders are provided.

## Requirements

- A C++17-compatible compiler
- CMake 3.16 or newer
- GNU Make for the commands below
- clang-format 18 for formatting (optional for building)
- Doxygen for generating documentation (optional for building)

## Quick start

Build and run with example gains:

```sh
make build
./build/app/equalizer input.raw 3 -2.5 output.raw
```

The application validates arguments, processes the input in 256-sample blocks, and writes little-endian PCM output.

## Usage

```text
equalizer <input_path> <gain_first_band> <gain_second_band> <output_path>
```

| Argument | Description |
| --- | --- |
| `input_path` | Path to the raw PCM input file |
| `gain_first_band` | First band gain, in dB (default center: 1 kHz) |
| `gain_second_band` | Second band gain, in dB (default center: 2 kHz) |
| `output_path` | Path intended for the processed raw PCM output |

Both gains must be in **[-12, +12] dB**, inclusive. Positive values boost, negative values attenuate, and `0` leaves the level unchanged. See [gain limits and clipping](#design-decision-gain-limits-and-clipping) for the rationale and output behavior.

You can also build and run through Make:

```sh
make run ARGS="input.raw 3 -2.5 output.raw"
```

## Build commands

| Command | Action |
| --- | --- |
| `make` or `make build` | Build the application without tests |
| `make build-tests` | Build the application and test executables |
| `make test` | Build and run tests |
| `make explore` | Run both Q and latency explorations |
| `make explore-latency` | Run the print-only block-size timing exploration |
| `make explore-q` | Build and run the print-only Q exploration |
| `make run` | Build and run the application; supply arguments with `ARGS` |
| `make clean` | Remove build and generated documentation directories |
| `make docs` | Generate Doxygen HTML documentation |
| `make show-docs` | Generate documentation and open it in your browser (Linux) |
| `make help` | Show available commands |

Application builds live in `build/app/`; builds with tests live in `build/tests/`. The `build/` directory is ignored by Git.

## Compile-time configuration

Band centers default to 1 kHz and 2 kHz and are defined in `AudioConfig.hpp`.
The application, center-gain tests, isolation tests, and explorations use these
constants. CLI argument order remains input, first-band gain, second-band gain,
output; usage messages and logs display the configured frequencies. The library
validates band centers against the sample rate during construction.

Fixed distant-frequency probes and the Python generator modes remain deliberate
test signals. Review probe choices if changing centers; historical measurements
below describe the default 1 kHz/2 kHz configuration.

Application settings belong to `equalizer_app::config`; library classes remain
in `audioeq`, and library gain limits remain in `audioeq::config`.

| File | Settings | Used by |
| --- | --- | --- |
| `inc/AudioConfig.hpp` | `kSampleRateHz`, `kProcessingBlockSize`, `kSelectedQ`, `kFirstBandFrequencyHz`, `kSecondBandFrequencyHz` | Application and regular DSP tests |
| `lib/audioeq/include/audioeq/Config.hpp` | `audioeq::config::kMinGainDb`, `kMaxGainDb` | Library, CLI validation, and gain-limit tests |

Edit the appropriate header and rebuild the library and all consumers together.
The library does not include the application configuration. Its existing
`Equalizer::kMinGainDb` and `kMaxGainDb` names alias the library settings, rather
than defining a second range.

Test signal generation and equalizer construction use the application's sample
rate. Regular processing tests use the selected Q and block size; isolation and
boundary tests use the configured gain limits. One-second response signals and
0.1-second settling windows are derived from the shared rate.

Test inputs such as +6 dB, silence, and invalid values remain deliberate test
scenarios, and acceptance tolerances remain independent. Explorations intentionally
compare candidate Q values and block sizes instead of only the selected settings.
Changing configuration does not guarantee tests pass: narrower gain limits can
reject a test scenario, and higher boosts may fail isolation or headroom checks.
Larger supported gain ranges require numerical validation.

The documented 48 kHz PCM format and Python generator describe the default
application configuration. If you change the sample rate, also update external
PCM generation/playback settings; raw PCM files contain no sample-rate metadata.

## Target-specific CMake settings

The root `CMakeLists.txt` lists application, test, and exploration targets in
`project_targets`. One `foreach` loop applies their common settings. It selects
C++17, disables compiler-specific C++ extensions for that target, and enables
`/W4` on MSVC or `-Wall -Wextra -Wpedantic` on GCC/Clang.

Warnings use `target_compile_options(... PRIVATE ...)`, so our warning policy
does not apply to GoogleTest or propagate to another application's targets.
GoogleTest retains its own compiler settings. No directory-wide warning flags
or global `CMAKE_CXX_EXTENSIONS` setting are imposed by this project.

The standalone library defines its own target settings in
`lib/audioeq/CMakeLists.txt`, so it does not depend on the root project. Its
include directory and C++17 requirement are `PUBLIC`: consumers need both to
use the headers. Its warning flags are `PRIVATE`, and `CXX_EXTENSIONS OFF`
controls compilation of the library itself, not consumers.

## Testing

```sh
make test
```

The current GoogleTest case, `EqualizerTest.ZeroGainPreservesInput`, verifies
that both bands at 0 dB preserve 48,000 generated samples within an absolute
error of 0.000001. It uses the shared `equalizer_app::config::kProcessingBlockSize`
(currently 256 samples), including the short final block. Four additional cases verify +6 dB and -6 dB at each band center,
with the other band at 0 dB and the selected Q = 8. They generate one second of a
0.1-amplitude tone, skip the first 4,800 samples (0.1 seconds), and compare
input/output RMS over the remaining interval. Measured gain must be within
0.1 dB of the request.

**Why skip settling samples when calculating RMS?**

A newly created biquad starts with zero filter history. When a sine wave begins,
the output initially contains a startup transient as well as the filtered tone.
That transient decays as the filter builds its history. Including it in RMS
would mix startup behavior with the steady-state level and can bias the measured
gain. The gain tests aim to measure the sustained tone's gain, not its startup.

We process the entire signal, but exclude the first **4,800 samples** from both
input and output RMS calculations. At 48 kHz this is **0.1 seconds**; the
remaining 43,200 samples provide a matching 0.9-second measurement window.
The measured gain is `20 * log10(output RMS / input RMS)`. This settling interval
is a test choice for the current configurations, not a universal filter rule.
Higher Q and lower center frequencies can require longer settling; changing
those settings substantially should prompt a check that extending the excluded
interval no longer materially changes the measured gain.

These samples are excluded only from the RMS measurement: the application does
not discard them, wait 0.1 seconds, or add a settling buffer. The interval is not
a measurement of processing latency. The zero-gain preservation test skips no
samples because it must verify unchanged output from the very first sample.

Isolation tests measure a 1 kHz or 2 kHz tone while adjusting only the other
band by +12 or -12 dB, using the same measurement interval and block size.
The proposed acceptance criterion is less than 0.5 dB absolute unwanted change
at the unchanged band center; the assignment does not specify this tolerance.

| Q | Maximum absolute unwanted change (measured) | Meets criterion? |
| --- | --- | --- |
| 2 | 1.45384 dB | No |
| 4 | 0.419606 dB | Yes |
| 8 | 0.109335 dB | Yes |

The table reports print-only Q exploration results, not separate regression
cases for each Q. The four isolation regression tests enforce the limit using
`equalizer_app::config::kSelectedQ`. Zero-gain, center-gain, isolation, and neighboring-frequency
tests use the application's shared Q and block-size settings from
`inc/AudioConfig.hpp` (currently Q = 8 and 256 samples).

`make test` builds the GoogleTest executables and runs it through CTest with
detailed, colored GoogleTest output. Nested Make directory messages are hidden.
Use `make test GTEST_COLOR=no` to disable test colors for plain-text logs. To see GoogleTest output directly, run:

```sh
./build/tests/equalizer_tests --gtest_filter=EqualizerTest.ZeroGainPreservesInput
```

Test builds use an installed GoogleTest CMake package if available. Otherwise,
CMake downloads GoogleTest v1.14.0 from its official repository and verifies
its SHA-256 hash. The first such build requires network access; the dependency
is cached under the test build directory. Application-only builds (`make build`)
do not require GoogleTest.

## Python PCM generator

[`tools/generate_pcm.py`](tools/generate_pcm.py) creates controlled audio inputs
for testing the complete application, including PCM reading, filtering, and
writing. It requires Python 3 with no additional packages.

### Generate a signal

With Python 3 installed, run any of these targets from the project root:

| Command | Generated file | Tones |
| --- | --- | --- |
| `make pcm-1khz` | `tools/tone_1000hz.pcm` | 1 kHz |
| `make pcm-2khz` | `tools/tone_2000hz.pcm` | 2 kHz |
| `make pcm-mixed` | `tools/mixed_tones.pcm` | 250 Hz, 1 kHz, 2 kHz, 6 kHz |

Each file contains five seconds of 48 kHz mono signed 16-bit little-endian
PCM with no header: 240,000 samples, or 480,000 bytes. Only Python's standard
library is needed. Override the interpreter with `PYTHON=/path/to/python3`.

The script divides a total amplitude of 0.1 among the selected tones, keeping
the combined input peak at approximately 10% of full scale and leaving boost
headroom. Single tones allow direct RMS gain measurements; mixed-tone total
RMS does not measure the gain of each frequency separately.

### Run Python directly

You can also run the script directly:

```sh
python3 tools/generate_pcm.py --mode mixed
python3 tools/generate_pcm.py tools/custom.pcm --mode 2khz
```

Without an explicit output path, files are placed beside the script regardless
of the working directory. The default mode is `1khz`. **Existing output files
are overwritten** when regenerating, so the Make targets can be run repeatedly.
This also applies to custom output paths. PCM files directly inside `tools/`
are ignored by Git.

### Process the generated audio

To boost the generated 1 kHz tone by 6 dB:

```sh
make build BUILD_TYPE=Release
./build/app/equalizer tools/tone_1000hz.pcm 6 0 tools/output_boost.pcm
```

## Run experimental measurements

```sh
make explore BUILD_TYPE=Release
```

This builds and runs the Q and latency explorations sequentially, without
running the regular CTest suite. `make explore` also works with the default
Debug build, but Release is recommended for representative processing times.
Use `make explore-q` or `make explore-latency BUILD_TYPE=Release` to run only
one exploration. Add `GTEST_COLOR=no` for plain-text output.

The latency exploration compares 256-, 512-, and 1,024-sample blocks using
`equalizer_app::config::kSelectedQ`. It restores its input outside each timed call and reports
one input block's duration plus the maximum observed DSP time over 100 calls.
This excludes output buffering, conversions, filter delay, and device overhead;
it is not a full end-to-end latency measurement or a worst-case guarantee.
Both exploration targets are excluded from the default build and CTest.

## Experimental Q exploration

Run the standalone Q comparison without running the regular tests:

```sh
make explore-q
# Optional optimized build and plain-text output:
make explore-q BUILD_TYPE=Release GTEST_COLOR=no
```

This builds `equalizer_q_exploration` and its dependencies in `build/tests/`,
then runs it directly. It prints four cross-band measurements for each candidate
Q in `test/EqualizerQExploration.cpp`. It is not registered with CTest and is
excluded from the default build, so `make test` does not run it. The program
uses GoogleTest to launch the exploration, but asserts no isolation threshold;
its “PASSED” output is not an acceptance verdict for a candidate Q.

## Additional regression coverage

The suite currently contains **21 GoogleTest cases** across two executables:
`equalizer_tests` for DSP and timing, and `pcm_conversion_tests` for sample
conversion. `make test` runs both through CTest.

| Source | Coverage | Cases |
| --- | --- | --- |
| `test/EqualizerTests.cpp` | Zero-gain preservation and four center-gain checks | 5 |
| `test/EqualizerFrequencyResponseTests.cpp` | Four cross-band isolation checks and two distant-frequency checks | 6 |
| `test/EqualizerEdgeCaseTests.cpp` | Silence under maximum boost, gain validation, empty buffers, and reset equivalence | 6 |
| `test/EqualizerLatencyTests.cpp` | Selected block duration plus observed DSP time | 1 |
| `test/PcmSigned16ConversionTests.cpp` | Positive/negative clipping and exact full-scale boundaries | 3 |

`test/TestSignalUtils.hpp` and `.cpp` provide signal generation, RMS, and dB
helpers in `testutils`, with no GoogleTest dependency. Measurement helpers
specific to frequency-response tests stay beside those tests.

The two distant-frequency cases loop over 18 tone/gain combinations in total;
these combinations are not separately registered GoogleTest cases. Q and
latency explorations are separate from this regression suite.

The PCM conversion cases check clipping at +2.0 and -2.0 and conversion at
exactly +1.0 and -1.0. These map to the PCM16 endpoints 32767 and -32768.
File-adapter tests and exhaustive PCM16 round-trip checks are not part of
this suite.

Passing these cases does not establish every possible numeric configuration
or continuous-spectrum behavior. Robustness at extreme positive Q/frequencies
near numeric limits, and the complete live latency budget, remain limitations.
Use Release builds for representative latency measurements.

<a id="design-decision-gain-limits-and-clipping"></a>

## Design decision: gain limits and clipping

### Allowed gain range

Each band accepts gains from **-12 dB to +12 dB**, including both endpoints.
`audioeq::config::kMinGainDb` and `kMaxGainDb` in the library configuration
define these limits. `Equalizer` exposes aliases used by command-line validation
and boundary tests. The range bounds the available boost and
cut and rejects arbitrarily large gain requests. It is a product/API constraint,
not a guarantee against clipping: clipping depends on the requested gain and
the input signal's level and frequency content.

### Where clipping happens

The equalizer processes normalized floating-point samples and does not clamp
its output to `[-1, +1]`. A positive gain can produce values outside this range,
especially when the input is already close to full scale. Keeping this behavior
in the DSP library lets another application reduce the level or apply a limiter
before converting the output.

`PcmSigned16Conversion::toFloat()` normalizes decoded signed PCM16 samples.
These functions operate on numeric values; byte order belongs to the file
adapter. Future formats can provide separate converters, such as
`PcmSigned32Conversion`; only signed PCM16 conversion is implemented today.

In this application, `PcmSigned16Conversion::fromFloat()` handles clipping at the
output conversion boundary:

| Processed sample | PCM16 result |
| --- | --- |
| At or above +1.0 | +32767 |
| At or below -1.0 | -32768 |
| Between -1.0 and +1.0 | Multiply by 32768 and truncate toward zero |

For inputs satisfying the conversion API's requirement that the sample is not
NaN, this produces a representable PCM16 value and avoids an out-of-range
float-to-integer conversion. Positive full scale is handled explicitly because
+32768 cannot be represented by a signed 16-bit integer.

### Practical trade-off

Samples exceeding full scale are **hard-clipped**, which can cause audible
distortion. The application currently has no limiter or automatic gain
compensation. Use quieter input or reduce the requested boost to leave
headroom; staying inside the allowed gain range alone does not prevent clipping.

## Design decision: why Q = 8

The application uses **Q = 8 for both bands** because it provides the best
isolation among the three tested candidates: Q = 2, 4, and 8. The assignment
asks for independent adjustments at 1 kHz and 2 kHz, so changing one band
should have as little effect as possible on the other band's center frequency.

### What the exploration measures

Run `make explore-q` to reproduce the comparison. For each Q, the exploration
generates a single tone at one band center, leaves that band's gain at 0 dB,
and sets the other band to +12 or -12 dB. It processes one second of audio at
48 kHz in 1,024-sample blocks and measures input/output RMS after excluding
4,800 settling samples. The exploration block size is independent of the
application's selected 256-sample block size.

The ideal measured change is **0 dB**. Positive results mean unwanted boost;
negative results mean unwanted attenuation. The absolute value measures how
far the supposedly unchanged tone moved from its original level.

| Input tone | Adjusted band and gain | Q = 2 | Q = 4 | Q = 8 |
| --- | --- | --- | --- | --- |
| 1,000 Hz | 2,000 Hz, +12 dB | +1.45383 dB | +0.419596 dB | +0.109335 dB |
| 1,000 Hz | 2,000 Hz, -12 dB | -1.45383 dB | -0.419606 dB | -0.109326 dB |
| 2,000 Hz | 1,000 Hz, +12 dB | +1.45384 dB | +0.419604 dB | +0.109324 dB |
| 2,000 Hz | 1,000 Hz, -12 dB | -1.45384 dB | -0.419597 dB | -0.109317 dB |
| **Maximum absolute change** | | **1.45384 dB** | **0.419606 dB** | **0.109335 dB** |

### Why we selected Q = 8

Q = 8 gives the smallest unwanted change in **all four scenarios**, covering
both frequency directions and both boost and cut. Its maximum absolute
cross-band change in dB is about **74% smaller than Q = 4** and **92% smaller
than Q = 2**. These percentages compare the measured dB magnitudes, not linear
amplitude or power.

Q = 4 and Q = 8 both meet our proposed 0.5 dB cross-band limit. Q = 8 provides
more margin: the worst measured change is approximately 0.109 dB, compared
with 0.420 dB for Q = 4. The 0.5 dB limit is our engineering criterion, not a
numerical tolerance specified by the assignment. The print-only exploration
itself does not enforce this limit; the isolation regression tests do.

Higher Q narrows each filter's affected frequency region, which suits the
priority of adjusting these two centers independently. Changing Q adds no
filters, stored state, per-sample arithmetic, or block buffering to this
implementation. We therefore retain Q = 8 rather than Q = 4 for the additional
measured isolation.

### Trade-offs and limits of this decision

Q = 8 is the best **tested candidate for cross-band center isolation**, not a
proof of the best Q for every application. Higher Q can increase ringing,
settling time, and frequency-dependent delay. These four measurements do not
establish the response at every neighboring frequency or total system latency.
The current neighboring-frequency checks are described below. They exercise
one adjusted band at a time; the complete latency budget remains outstanding.

The 0.1-second interval excluded from RMS measurements is test settling time,
not an added application buffer or a measured latency. Application settings
remain Q = 8 and 256 samples per block.

## Neighboring-frequency validation at Q = 8

Two cases in `test/EqualizerFrequencyResponseTests.cpp` check distant tones
using the shared selected Q and block size (currently 8 and 256 samples):

| Test in `EqualizerNeighborResponse` | Adjusted band | Probe frequencies (Hz) | Tone/gain combinations |
| --- | --- | --- | --- |
| `FirstBandDoesNotAffectDistantFrequencies` | 1 kHz | 250, 500, 3,000, 4,000, 6,000 | 10 |
| `SecondBandDoesNotAffectDistantFrequencies` | 2 kHz | 250, 500, 4,000, 6,000 | 8 |

Each probe is measured with +12 dB and -12 dB on the adjusted band while the
other band stays at 0 dB. Each measurement uses one second of 0.1-amplitude
audio and matching input/output RMS windows after excluding 4,800 settling
samples. The absolute change must be less than **0.5 dB**, a project criterion
rather than an assignment-specified tolerance.

These 18 measurements check only the listed frequencies. They do not establish
a continuous-spectrum bound, the response at closer transition frequencies,
or simultaneous-band behavior.

## Processing timing and buffer selection

The application selects **256 samples per block**, shared with the latency test
through `equalizer_app::config::kProcessingBlockSize` in `inc/AudioConfig.hpp`. At 48 kHz,
one block represents:

```text
256 / 48000 * 1000 = 5.33333 ms of audio
```

### Exploration results

The following results were reported by the latency exploration in a **Debug
build**. Each candidate uses the selected Q, a representative mixed tone,
and 100 measured processing calls. Input is refreshed outside the timed calls.
All table values are in milliseconds.

| Block size (samples) | One input block duration | Maximum observed DSP time | One input block + DSP estimate |
| --- | --- | --- | --- |
| 256 | 5.33333 | 0.006891 | 5.34022 |
| 512 | 10.6667 | 0.024168 | 10.6908 |
| 1,024 | 21.3333 | 0.025981 | 21.3593 |

These are observations from one run, not guaranteed upper bounds. The maximum
can vary with scheduling, machine load, compiler, and hardware; its differences
between block sizes should not be interpreted as exact CPU scaling. To repeat
the exploration with optimizations enabled, run:

```sh
make explore-latency BUILD_TYPE=Release
```

### Why we chose 256 samples

Among the three tested sizes, 256 has the lowest buffering contribution:
**5.33 ms**, compared with **10.67 ms** for 512 and **21.33 ms** for 1,024.
The observed DSP time of **0.006891 ms** used about **0.13%** of the 5.33333 ms
block interval, leaving substantial processing headroom in this run. The DSP
work was much smaller than the buffering contribution even in this Debug build.

We therefore choose 256 to reduce buffering delay while retaining measured DSP
headroom. The trade-off is more frequent processing and adapter calls: about
187.5 calls per second at 48 kHz, versus 93.75 for 512 samples and 46.875 for
1,024. Adapter overhead and power consumption were not measured, so this is a
supported choice among the tested sizes, not proof of a universal optimum.
The file-processing loop still runs as fast as possible, without sleeps.

### What the estimate does not prove

The printed estimate includes **one input block plus observed DSP execution**.
It excludes output buffering, PCM conversion, frequency-dependent filter delay,
and driver/device overhead. Although all three estimates are below 100 ms,
these measurements alone do not prove the full end-to-end latency requirement.
With one input and one output block buffered, 256 samples would contribute
approximately **10.67 ms of buffering**, before the other contributions.
The complete latency budget remains to be assessed.

The `EqualizerLatency` regression case checks the selected block duration plus
maximum observed processing time against 100 ms over 100 calls. Run it with:

```sh
make test BUILD_TYPE=Release
./build/tests/equalizer_tests --gtest_filter='EqualizerLatency.*'
```

A passing case checks only that partial estimate. Use optimized measurements
on the intended target and account for the full adapter before claiming a
real-time guarantee; the supplied Debug results are not Release measurements.

## Code formatting

The shared `.clang-format` file defines the style. Use **clang-format 18** for consistent results across contributors.

Install it on Ubuntu:

```sh
sudo apt install clang-format-18
```

Format files or check them without making changes:

```sh
make format
make format-check
```

A successful formatting check prints the command and exits without errors. If your version 18 executable has a different name or location, override it:

```sh
make format CLANG_FORMAT=/path/to/clang-format
```

Keep `.clang-format` and `Makefile` in version control so everyone shares the same rules and commands.

## Documentation

Doxygen generates the project reference from source comments and this README.
On Ubuntu, install it and generate the pages with:

```sh
sudo apt install doxygen
make docs
```

Run `make show-docs` to regenerate the documentation and open it in your default
browser using Linux's `xdg-open`, or open `build/docs/html/index.html` manually.
Generated pages are ignored by
Git and removed by `make clean`; commit `Doxyfile` and the documented sources.
If needed, select a different executable with `make docs DOXYGEN=/path/to/doxygen`.

The reference covers the DSP core, command-line validation, PCM conversion,
file handling, logging, application startup, and test helpers.

Use `@brief` for a short description, `@param` for each argument, `@return` for
returned values, and `@throws` for expected exceptions. Keep public API contracts
in headers and private helper documentation beside their implementations.
Documentation warnings fail `make docs` so broken comments can be caught early.

## Logging

Include `Logger.hpp` to use `LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, and `LOG_ERROR`. Messages go to standard error with a local timestamp, level, source filename, and line number.
The `LOG_*` macros capture the location of the call automatically.

```text
[2026-09-23 14:30:00] [INFO] [main.cpp:25] Application started
```

The logger defaults to `INFO`; the current `main()` explicitly enables `DEBUG`.
Set the minimum level in code with:

```cpp
Logger::setLevel(Logger::Level::Debug);
LOG_DEBUG("Debug logging enabled");
```

## Project layout

```text
Equalizer/
├── lib/audioeq/        # Standalone DSP library and its CMakeLists.txt
│   ├── include/audioeq/ # Public Equalizer.hpp and Biquad.hpp
│   └── src/            # DSP implementations
├── inc/                # Application and adapter headers
├── src/                # Application and component implementations
├── tools/              # Python PCM generator and generated audio
├── test/               # Tests
├── .clang-format       # Shared formatting rules
├── .gitignore          # Files excluded from Git
├── CMakeLists.txt      # Build and test configuration
├── Doxyfile            # Doxygen documentation configuration
├── Makefile            # Build, test, formatting, and documentation commands
└── README.md
```

### Shared selected Q

`equalizer_app::config::kSelectedQ` in `inc/AudioConfig.hpp` is the authoritative application
Q. The application, selected-band correctness tests, and latency test use it.
Changing this constant changes the configuration exercised by those tests;
acceptance tolerances remain independent. The print-only Q exploration retains
its own candidate list. A configuration fails when it violates an asserted
requirement, not merely because it differs from 8; passing is not proof of
all possible response or latency properties.
