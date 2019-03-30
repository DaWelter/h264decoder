cmake_minimum_required(VERSION 2.8)
project(python_h264decoder)

set(Python_ADDITIONAL_VERSIONS "2.7" CACHE STRING "Which python version should be taken into account when looking for python")
find_package(PythonInterp REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(Boost REQUIRED COMPONENTS "python")
message (WARNING "This script cannot automatically determine the correct boost python library for your python version. Please check.")
# TODO: 

include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

if (WIN32)
	  set (LIBAV_INCLUDE_DIR CACHE PATH "Where to find libav headers")
	    set (LIBAV_LIBRARY_DIR CACHE PATH "Where to find libav libs")
	      include_directories(${LIBAV_INCLUDE_DIR})
	        link_directories(${LIBAV_LIBRARY_DIR})
	else (WIN32)
		  add_compile_options ("-std=c++0x")
	  endif (WIN32)

	  set(LIBAV_LIBRARIES avcodec swscale avutil)

	  add_executable(h264decoder_test1 h264decoder.cpp h264decoder_test1.cpp)
	  target_link_libraries(h264decoder_test1 ${LIBAV_LIBRARIES})

	  add_library(h264decoder SHARED h264decoder.cpp h264decoder_python.cpp)
	  target_link_libraries(h264decoder ${LIBAV_LIBRARIES} ${Boost_PYTHON_LIBRARY_RELEASE} ${PYTHON_LIBRARIES})

	  if (WIN32)
		    set_target_properties(h264decoder PROPERTIES PREFIX "lib")
		      set_target_properties(h264decoder PROPERTIES SUFFIX ".pyd")
	      else()
		        add_custom_command(TARGET h264decoder POST_BUILD
				              COMMAND ${CMAKE_COMMAND} -E create_symlink 
					                    ${CMAKE_BINARY_DIR}/libh264decoder.so ${CMAKE_SOURCE_DIR}/libh264decoder.so)
					    endif()
					    install(TARGETS h264decoder LIBRARY DESTINATION . ARCHIVE DESTINATION .)
