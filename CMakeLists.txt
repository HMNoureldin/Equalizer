cmake_minimum_required(VERSION 3.16)

project(Equalizer VERSION 0.1.0 LANGUAGES CXX)

# Reusable DSP library, independently buildable from lib/audioeq.
add_subdirectory(lib/audioeq)

# Application and file adapters depend on the DSP core.
add_executable(equalizer
    src/main.cpp
    src/CommandLine.cpp
    src/PcmSigned16Conversion.cpp
    src/RawPcmSigned16File.cpp
    src/Logger.cpp
)
set(project_targets equalizer)
target_link_libraries(equalizer PRIVATE audioeq::audioeq)

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
        test/EqualizerEdgeCaseTests.cpp
        test/EqualizerFrequencyResponseTests.cpp
        test/EqualizerLatencyTests.cpp
        test/TestSignalUtils.cpp
    )
    target_link_libraries(equalizer_tests PRIVATE audioeq::audioeq GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(equalizer_tests)

    # Optional print-only exploration, built explicitly and not registered in CTest.
    add_executable(equalizer_q_exploration EXCLUDE_FROM_ALL
        test/EqualizerQExploration.cpp
        test/TestSignalUtils.cpp
    )
    target_link_libraries(equalizer_q_exploration
        PRIVATE audioeq::audioeq GTest::gtest_main)

    add_executable(equalizer_latency_exploration EXCLUDE_FROM_ALL
        test/EqualizerLatencyExploration.cpp
        test/TestSignalUtils.cpp
    )
    target_link_libraries(equalizer_latency_exploration
        PRIVATE audioeq::audioeq GTest::gtest_main)

    # PCM conversion tests stay separate from the DSP test executable.
    add_executable(pcm_conversion_tests
        test/PcmSigned16ConversionTests.cpp src/PcmSigned16Conversion.cpp)
    target_link_libraries(pcm_conversion_tests PRIVATE GTest::gtest_main)
    gtest_discover_tests(pcm_conversion_tests)

    list(APPEND project_targets
        equalizer_tests pcm_conversion_tests
        equalizer_q_exploration equalizer_latency_exploration
    )
endif()

# Common settings for our executables only (not GoogleTest).
foreach(target IN LISTS project_targets)
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/inc")
    target_compile_features(${target} PRIVATE cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()
