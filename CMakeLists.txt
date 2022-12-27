cmake_minimum_required(VERSION 3.10)
project(h264decoder)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set (BuildExamples False CACHE BOOL "Build the examples")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)
if (NOT WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wno-deprecated-declarations")
endif()

# Not working on Windows
#   find_package(PkgConfig REQUIRED)
#   pkg_check_modules(libswscale REQUIRED libswscale)
#   pkg_check_modules(libavutil REQUIRED libavutil)
#   pkg_check_modules(libavcodec REQUIRED libavcodec)
#
# Not on Ubuntu ...
# find_package(FFMPEG REQUIRED swscale avutil avcodec)

# So we do it more low level
find_path( AVCODEC_INCLUDE_DIR libavcodec/avcodec.h )
find_library( AVCODEC_LIBRARY avcodec )
find_path( AVUTIL_INCLUDE_DIR libavutil/avutil.h )
find_library( AVUTIL_LIBRARY avutil )
find_path( SWSCALE_INCLUDE_DIR libswscale/swscale.h )
find_library( SWSCALE_LIBRARY swscale )

set(FFMPEG_LIBRARIES ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY})
set(FFMPEG_INCLUDE_DIR ${AVCODEC_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR})


find_package(pybind11)
if(pybind11_FOUND)
  message("Using existing pybind11 v${pybind11_VERSION}")
else()
  message("Fetching pybind11")
  include(FetchContent)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    # Recent PyBind11 version is required for Python 3.11 support.
    GIT_TAG v2.10.2)
  FetchContent_MakeAvailable(pybind11)
endif()


add_library(h264decoderlib STATIC src/h264decoder.cpp src/h264decoder.hpp)
target_sources(h264decoderlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/h264decoder.hpp)
target_link_libraries(h264decoderlib PUBLIC ${FFMPEG_LIBRARIES})
target_include_directories(h264decoderlib PUBLIC ${FFMPEG_INCLUDE_DIR})
target_include_directories(h264decoderlib PUBLIC "src")

pybind11_add_module(h264decoder src/h264decoder_python.cpp)
target_link_libraries(h264decoder PRIVATE h264decoderlib)

if (BuildExamples)
  add_subdirectory(examples)
endif ()
