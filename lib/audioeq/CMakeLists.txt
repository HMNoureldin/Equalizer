cmake_minimum_required(VERSION 3.16)

project(audioeq VERSION 0.1.0 LANGUAGES CXX)

# Standalone DSP only: no application, file adapters, or test dependencies.
add_library(equalizer_core STATIC
    src/Biquad.cpp
    src/Equalizer.cpp
)
add_library(audioeq::audioeq ALIAS equalizer_core)

target_include_directories(equalizer_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(equalizer_core PUBLIC cxx_std_17)
set_target_properties(equalizer_core PROPERTIES CXX_EXTENSIONS OFF)
