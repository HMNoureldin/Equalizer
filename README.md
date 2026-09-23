# Equalizer

**Author:** H.Noureldin  
**Email:** [heshamnoureldin2017@gmail.com](mailto:heshamnoureldin2017@gmail.com)

A C++17 command-line application for applying gain at **1 kHz** and **2 kHz** to raw PCM audio.

> **Development status:** Argument parsing, gain validation, logging, and build tooling are implemented. Audio processing and output-file generation are not implemented yet.

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

The application currently validates the arguments and logs its status; it does not read or write audio files.

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

The argument-parsing tests cover valid inputs, gain boundaries, missing or extra arguments, invalid numbers, and out-of-range gains. CTest displays failure details if a test fails.

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

The reference covers command-line options, gain validation, logging, application
startup, and test helpers. The Biquad, Equalizer, and RawPcmFile files are explicitly
marked as placeholders until their APIs and implementations are added.

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
