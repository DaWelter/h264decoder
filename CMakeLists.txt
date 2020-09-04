cmake_minimum_required(VERSION 3.14)
project(h264decoder)

set(CMAKE_BUILD_TYPE Debug)

find_package(FFMPEG REQUIRED swscale avutil avcodec)

find_package(pybind11)
if(pybind11_FOUND)
  message("Using existing pybind11 v${pybind11_VERSION}")
else()
  message("Fetching latest pybind11")
  include(FetchContent)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG v2.5.0)
  FetchContent_MakeAvailable(pybind11)
endif()

link_libraries(${FFMPEG_LIBRARIES})
include_directories(${FFMPEG_INCLUDE_DIR})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-Wall -Wpedantic")

pybind11_add_module(h264decoder h264decoder.cpp h264decoder_python.cpp)
#add_executable(h264decoder_test1 h264decoder.cpp h264decoder_test1.cpp)
