cmake_minimum_required(VERSION 3.11)
project(python_h264decoder)

set(CMAKE_BUILD_TYPE Debug)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libswscale REQUIRED libswscale)
pkg_check_modules(libavutil REQUIRED libavutil)
pkg_check_modules(libavcodec REQUIRED libavcodec)

include(FetchContent)
FetchContent_Declare(pybind11 GIT_REPOSITORY https://github.com/pybind/pybind11)
FetchContent_MakeAvailable(pybind11)

link_libraries(${libswscale_LIBRARIES} ${libavutil_LIBRARIES} ${libavcodec_LIBRARIES})
include_directories(${libswscale_INCLUDE_DIRS} ${libavutil_INCLUDE_DIRS} ${libavcodec_INCLUDE_DIRS})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-Wall -Wpedantic")

pybind11_add_module(libh264decoder h264decoder.cpp h264decoder_python.cpp)
#add_executable(h264decoder_test1 h264decoder.cpp h264decoder_test1.cpp)
