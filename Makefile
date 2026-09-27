.DEFAULT_GOAL := build

BUILD_TYPE ?= Debug
JOBS ?= 2
ARGS ?=
CLANG_FORMAT ?= clang-format-18
DOXYGEN ?= doxygen
PYTHON ?= python3
GTEST_COLOR ?= yes
FORMAT_FILES := $(wildcard inc/*.hpp src/*.cpp test/*.cpp test/*.hpp lib/audioeq/include/audioeq/*.hpp lib/audioeq/src/*.cpp)

.PHONY: pcm-1khz pcm-2khz pcm-mixed build build-tests test explore explore-q explore-latency run clean format format-check docs show-docs help

# Generate audio fixtures without building the C++ application.
pcm-1khz:
	$(PYTHON) tools/generate_pcm.py --mode 1khz

pcm-2khz:
	$(PYTHON) tools/generate_pcm.py --mode 2khz

pcm-mixed:
	$(PYTHON) tools/generate_pcm.py --mode mixed

# Keep builds with and without tests separate.
build:
	cmake -S . -B build/app -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=OFF
	cmake --build build/app --parallel $(JOBS)

build-tests:
	cmake -S . -B build/tests -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=ON
	cmake --build build/tests --parallel $(JOBS)

# Inherited by the build prerequisite and its nested Make processes.
test: MAKEFLAGS += --no-print-directory
test: build-tests
	@cmake -E env GTEST_COLOR=$(GTEST_COLOR) ctest --test-dir build/tests --verbose --output-on-failure

# Build only the requested exploration and its dependencies; do not run CTest.
explore-q: MAKEFLAGS += --no-print-directory
explore-q:
	cmake -S . -B build/tests -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=ON
	cmake --build build/tests --target equalizer_q_exploration --parallel $(JOBS)
	@cmake -E env GTEST_COLOR=$(GTEST_COLOR) ./build/tests/equalizer_q_exploration

# Run both explorations sequentially to avoid competing timing workloads.
explore: MAKEFLAGS += --no-print-directory
explore:
	cmake -S . -B build/tests -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=ON
	cmake --build build/tests --target equalizer_q_exploration equalizer_latency_exploration --parallel $(JOBS)
	@cmake -E env GTEST_COLOR=$(GTEST_COLOR) ./build/tests/equalizer_q_exploration
	@cmake -E env GTEST_COLOR=$(GTEST_COLOR) ./build/tests/equalizer_latency_exploration

explore-latency: MAKEFLAGS += --no-print-directory
explore-latency:
	cmake -S . -B build/tests -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=ON
	cmake --build build/tests --target equalizer_latency_exploration --parallel $(JOBS)
	@cmake -E env GTEST_COLOR=$(GTEST_COLOR) ./build/tests/equalizer_latency_exploration

run: build
	./build/app/equalizer $(ARGS)

# Remove only the build directories managed by this Makefile.
clean:
	cmake -E rm -rf build/app build/tests build/docs

docs:
	$(DOXYGEN) Doxyfile

show-docs: docs
	xdg-open build/docs/html/index.html

format:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

help:
	@echo "make pcm-1khz     Generate tools/tone_1000hz.pcm"
	@echo "make pcm-2khz     Generate tools/tone_2000hz.pcm"
	@echo "make pcm-mixed    Generate tools/mixed_tones.pcm"
	@echo "make build        Build application without tests (default)"
	@echo "make build-tests  Build application and tests"
	@echo "make test         Build and run tests with detailed GoogleTest output"
	@echo "make explore      Run Q and latency explorations (use BUILD_TYPE=Release for timing)"
	@echo "make explore-latency  Run the latency exploration only"
	@echo "make explore-q    Build and run the Q exploration only"
	@echo "make run          Build and run application (ARGS='...' optional)"
	@echo "make clean        Remove build and generated documentation directories"
	@echo "make docs         Generate Doxygen HTML documentation"
	@echo "make show-docs    Generate documentation and open it in your browser (Linux)"
	@echo "make format       Format C++ files with clang-format 18"
	@echo "make format-check Check formatting without modifying files"
