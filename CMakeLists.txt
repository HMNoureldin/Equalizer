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
    # Use an installed package when available; otherwise fetch a pinned release.
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        if(POLICY CMP0135)
            cmake_policy(SET CMP0135 NEW)
        endif()
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        if(MSVC)
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        endif()
        FetchContent_Declare(googletest
            URL https://codeload.github.com/google/googletest/tar.gz/refs/tags/v1.14.0
            URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
        )
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(equalizer_tests
        test/EqualizerTests.cpp
        test/EqualizerFrequencyResponseTests.cpp
        test/EqualizerLatencyTests.cpp
        test/TestSignalUtils.cpp
    )
    target_link_libraries(equalizer_tests PRIVATE equalizer_core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(equalizer_tests)

    # Optional print-only exploration, built explicitly and not registered in CTest.
    add_executable(equalizer_q_exploration EXCLUDE_FROM_ALL
        test/EqualizerQExploration.cpp
        test/TestSignalUtils.cpp
    )
    target_link_libraries(equalizer_q_exploration
        PRIVATE equalizer_core GTest::gtest_main)

    add_executable(equalizer_latency_exploration EXCLUDE_FROM_ALL
        test/EqualizerLatencyExploration.cpp
        test/TestSignalUtils.cpp
    )
    target_link_libraries(equalizer_latency_exploration
        PRIVATE equalizer_core GTest::gtest_main)

    # File/conversion tests stay separate from the DSP-only test executable.
    add_executable(audio_adapter_tests
        test/AudioAdapterTests.cpp src/RawPcmFile.cpp src/PcmConversion.cpp)
    target_include_directories(audio_adapter_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/inc")
    target_compile_features(audio_adapter_tests PRIVATE cxx_std_17)
    target_link_libraries(audio_adapter_tests PRIVATE GTest::gtest_main)
    gtest_discover_tests(audio_adapter_tests)
endif()
