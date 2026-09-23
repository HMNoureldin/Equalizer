.DEFAULT_GOAL := build

BUILD_TYPE ?= Debug
JOBS ?= 2
ARGS ?=
CLANG_FORMAT ?= clang-format-18
DOXYGEN ?= doxygen
FORMAT_FILES := $(wildcard inc/*.hpp src/*.cpp test/*.cpp)

.PHONY: build build-tests test run clean format format-check docs show-docs help

# Keep builds with and without tests separate.
build:
	cmake -S . -B build/app -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=OFF
	cmake --build build/app --parallel $(JOBS)

build-tests:
	cmake -S . -B build/tests -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=ON
	cmake --build build/tests --parallel $(JOBS)

test: build-tests
	ctest --test-dir build/tests --output-on-failure

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
	@echo "make build        Build application without tests (default)"
	@echo "make build-tests  Build application and tests"
	@echo "make test         Build and run tests"
	@echo "make run          Build and run application (ARGS='...' optional)"
	@echo "make clean        Remove build and generated documentation directories"
	@echo "make docs         Generate Doxygen HTML documentation"
	@echo "make show-docs    Generate documentation and open it in your browser (Linux)"
	@echo "make format       Format C++ files with clang-format 18"
	@echo "make format-check Check formatting without modifying files"
