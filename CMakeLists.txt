cmake_minimum_required(VERSION 3.16)

project(Equalizer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Reusable DSP library, independent of CLI, file I/O, and logging.
add_library(equalizer_core STATIC
    src/Biquad.cpp
    src/Equalizer.cpp
)

target_include_directories(equalizer_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
)
target_compile_features(equalizer_core PUBLIC cxx_std_17)

# Application and file adapters depend on the DSP core.
add_executable(equalizer
    src/main.cpp
    src/CommandLine.cpp
    src/PcmConversion.cpp
    src/RawPcmFile.cpp
    src/Logger.cpp
)
target_link_libraries(equalizer PRIVATE equalizer_core)

# Tests (disable with -DBUILD_TESTING=OFF).
include(CTest)

if(BUILD_TESTING)
    add_executable(equalizer_tests test/EqualizerTests.cpp)
    target_link_libraries(equalizer_tests PRIVATE equalizer_core)
    add_test(NAME EqualizerZeroGain COMMAND equalizer_tests)
endif()
