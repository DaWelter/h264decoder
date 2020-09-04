cmake_minimum_required(VERSION 3.14)
project(examples)

add_executable(extract_frames extract_frames.cpp)
target_link_libraries(extract_frames h264decoderlib)