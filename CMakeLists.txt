cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(JPEGDecoder)

find_path (JPEG_INCLUDES jpeglib.h)
find_library (JPEG_LIBRARIES NAMES jpeg)
find_path (FFTW_INCLUDES fftw3.h)
find_library (FFTW_LIBRARIES NAMES fftw3)

message(STATUS "Path to JPEG includes: ${JPEG_INCLUDES}")
message(STATUS "Path to JPEG library: ${JPEG_LIBRARIES}")
message(STATUS "Path to FFTW includes: ${FFTW_INCLUDES}")
message(STATUS "Path to FFTW library: ${FFTW_LIBRARIES}")

add_library(jpeg_decoder 
    Source/Decoder.cpp
    Source/DecoderData.cpp
    Source/FFT.cpp
    Source/Huffman.cpp
    Source/Section.cpp
    Source/SectionDetector.cpp
)

target_include_directories(jpeg_decoder PUBLIC Include ${FFTW_INCLUDE_DIRS} ${JPEG_INCLUDES})
target_link_libraries(jpeg_decoder ${FFTW_LIBRARIES} ${JPEG_LIBRARIES})

add_executable(test_jpeg_decoder 
    Test/Main.cpp
)
target_include_directories(test_jpeg_decoder PUBLIC include)
target_link_libraries(test_jpeg_decoder jpeg_decoder)
target_compile_definitions(test_jpeg_decoder PUBLIC IMAGE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Images/")

