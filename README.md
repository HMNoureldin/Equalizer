# Equalizer

**Author:** H.Noureldin  
**Email:** [heshamnoureldin2017@gmail.com](mailto:heshamnoureldin2017@gmail.com)

A C++17 command-line application for applying gain at **1 kHz** and **2 kHz** to raw PCM audio.

> **Development status:** Streaming PCM processing, configurable two-band DSP, file output, and automated tests are implemented. The complete end-to-end latency assessment remains outstanding.

## Audio format

Input and output use headerless signed 16-bit little-endian PCM, mono,
at 48 kHz. Byte order is encoded explicitly, independently of the host.
A final short block of complete samples is processed normally. An odd
file length (a trailing incomplete sample) is rejected with an error;
an error can leave a partial output file.

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
equalizer <input_path> <gain_1kHz> <gain_2kHz> <output_path>
```

| Argument | Description |
| --- | --- |
| `input_path` | Path to the raw PCM input file |
| `gain_1kHz` | Gain at 1 kHz, in dB |
| `gain_2kHz` | Gain at 2 kHz, in dB |
| `output_path` | Path intended for the processed raw PCM output |

Both gains must be in **[-12, +12] dB**, inclusive. Positive values boost, negative values attenuate, and `0` leaves the level unchanged.

You can also build and run through Make:

```sh
make run ARGS="input.raw 3 -2.5 output.raw"
```

## Build commands

| Command | Action |
| --- | --- |
| `make` or `make build` | Build the application without tests |
| `make build-tests` | Build the application and test executable |
| `make test` | Build and run tests |
| `make explore-q` | Build and run the print-only Q exploration |
| `make run` | Build and run the application; supply arguments with `ARGS` |
| `make clean` | Remove build and generated documentation directories |
| `make docs` | Generate Doxygen HTML documentation |
| `make show-docs` | Generate documentation and open it in your browser (Linux) |
| `make help` | Show available commands |

Application builds live in `build/app/`; builds with tests live in `build/tests/`. The `build/` directory is ignored by Git.

## Testing

```sh
make test
```

The current GoogleTest case, `EqualizerTest.ZeroGainPreservesInput`, verifies
that both bands at 0 dB preserve 48,000 generated samples within an absolute
error of 0.000001. It processes 1,024-sample blocks, including the short final
block. Four additional cases verify +6 dB and -6 dB at each band center,
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

The Q=2 test characterizes a rejected baseline by asserting that leakage
exceeds the limit; its passing does not mean Q=2 meets the requirement.
The Q=4 and Q=8 comparison tests retain their fixed values and assert that
all four cases stay below the limit. Zero-gain and gain-accuracy tests use
the application's selected Q=8.

Block-consistency tests compare separate equalizer instances processing the
same 48,017-sample signal in one call or blocks of 256, 512, and 1,024 samples.
They use Q=8, +6 dB at 1 kHz, and -3 dB at 2 kHz, preserve state between calls,
and require sample-by-sample agreement within 0.000001. The signal includes a
quiet mixed tone, an initial impulse, and a silence interval. Each block size
has a short final block. These are correctness tests, not timing benchmarks.
The application uses 256 samples per block (5.33 ms of audio at 48 kHz),
selected from the Release DSP timing measurements below. The correctness
comparisons retain all three block sizes.

`make test` builds the GoogleTest executable and runs it through CTest with
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
`audioeq::kSelectedQ`. It restores its input outside each timed call and reports
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

The Release suite currently discovers 66 GoogleTest cases across two executables:
`equalizer_tests` for DSP and timing, and `audio_adapter_tests` for file handling
and sample conversion. `make test` runs both through CTest.

Test source organization:
- `test/TestSignalUtils.hpp` and `.cpp`: signal generation, RMS, and dB helpers
  in `testutils`, with no GoogleTest dependency.
- `test/EqualizerTests.cpp`: general correctness tests and their assertion helpers.
- `test/EqualizerFrequencyResponseTests.cpp`: cross-band isolation tests and their measurement helper.
- `test/EqualizerLatencyTests.cpp`: selected-buffer plus observed DSP-time check.
- `test/AudioAdapterTests.cpp`: PCM conversion and file-handling tests.

Correctness and timing sources still build into `equalizer_tests`, so existing
`make test` remains unchanged; the timing suite is now `EqualizerLatency`.


- Simultaneous bands: all four +/-12 dB pairings at both center frequencies,
  with 0.01-amplitude tones to avoid clipping. A 0.2 dB center-gain tolerance
  includes the known cross-band contribution; this is a project criterion.
- Core edge cases: silence under boost, reset matching a fresh instance,
  invalid constructor parameters and gain updates, rejected updates preserving
  state, no-op buffers, and absolute gain changes retaining history.
- PCM conversion: exhaustive round trips for all 65,536 signed 16-bit values,
  scaling, truncation, clipping, and endpoints.
- File adapter: independently specified little-endian bytes for all PCM16
  values, empty/exact/short reads, guard samples around read buffers, incomplete
  samples, invalid arguments, all opening outcomes, retry after opening failure,
  same-file protection (including hard and symbolic links), and finalization.

Each file test owns an isolated temporary directory. Link tests skip if the
filesystem or permissions do not allow link creation. The final buffered-write
failure test uses Linux `/dev/full` and skips where that facility is unavailable.
The tests do not require permanent audio fixtures or change user files.

Passing these cases does not establish every possible numeric configuration
or continuous-spectrum behavior. Robustness at extreme positive Q/frequencies
near numeric limits, and the complete live latency budget, remain limitations.
Use Release builds for representative latency measurements.

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
Neighboring-frequency results and simultaneous-band regression tests are
documented elsewhere in this README; the complete latency budget remains
outstanding.

The 0.1-second interval excluded from RMS measurements is test settling time,
not an added application buffer or a measured latency. Application settings
remain Q = 8 and 256 samples per block.

## Neighboring-frequency validation at Q = 8

The parameterized `SelectedQ/EqualizerNeighborResponse` suite measures 32 combinations:
eight tones (250, 500, 750, 1,500, 2,500, 3,000, 4,000, 6,000 Hz), either band
adjusted, and gains of -12/+12 dB. The other band stays at 0 dB. It uses one
second of 0.1-amplitude audio, 256-sample blocks, and matching RMS measurement
intervals after skipping the first 0.1 seconds.

For these checks we define outer-band probes as frequencies at or below
center/1.5 or at or above center*1.5. They must change by less than 0.5 dB.
Closer probes are transition-region measurements, checked for finite output
and headroom but not asserted below 0.5 dB. This is a chosen engineering
boundary for sampled checks, not a specified assignment bandwidth or a
measured cutoff. Passing does not mean all frequencies are unchanged.

Maximum absolute changes across boost and cut in the measured run:

| Probe (Hz) | Adjusting 1 kHz (dB) | Adjusting 2 kHz (dB) |
| --- | --- | --- |
| 250 | 0.018 | 0.004 |
| 500 | 0.110 | 0.018 |
| 750 | 0.677 (transition) | 0.047 |
| 1,500 | 0.345 | 0.668 (transition) |
| 2,500 | 0.056 | 1.053 (transition) |
| 3,000 | 0.034 | 0.336 |
| 4,000 | 0.017 | 0.105 |
| 6,000 | 0.007 | 0.032 |

All 26 outer-band combinations meet the limit; the largest change is 0.345 dB.
The six transition combinations reach 1.053 dB and would fail a blanket 0.5 dB
requirement. We retain Q=8 and 256-sample buffers for the documented bandwidth
interpretation. If those closer frequencies must also stay below 0.5 dB, this
configuration needs reconsideration. These discrete single-band measurements
do not establish a continuous-spectrum bound or simultaneous-band behavior.

## Processing timing and buffer selection

The application selects **256 samples per block**, shared with the latency test
through `audioeq::kProcessingBlockSize` in `inc/AudioConfig.hpp`. At 48 kHz,
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
Previous block-consistency checks also found identical output across these
sizes, so reducing the block size did not change the filtering result.

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
├── inc/                # C++ headers
├── src/                # Application and component implementations
├── test/               # Tests
├── .clang-format       # Shared formatting rules
├── .gitignore          # Files excluded from Git
├── CMakeLists.txt      # Build and test configuration
├── Doxyfile            # Doxygen documentation configuration
├── Makefile            # Build, test, formatting, and documentation commands
└── README.md
```

### Shared selected Q

`audioeq::kSelectedQ` in `inc/AudioConfig.hpp` is the authoritative application
Q. The application, selected-band correctness tests, and latency test use it.
Changing this constant changes the configuration exercised by those tests;
acceptance tolerances remain independent. The print-only Q exploration retains
its own candidate list. A configuration fails when it violates an asserted
requirement, not merely because it differs from 8; passing is not proof of
all possible response or latency properties.
