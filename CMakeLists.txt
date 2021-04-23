cmake_minimum_required(VERSION 3.12.0)
project(raspiloggeroled)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -mfpu=vfp -mfloat-abi=hard -mtune=arm1176jzf-s -lrf24-bcm -lrf24network -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(THREADS_PREFER_PTHREAD_FLAG ON)

#--------------------------------------------------------------------------

add_library(SSD1306 STATIC lib/FileDescriptor.cxx
						   lib/OledHardware.cxx
						   lib/OledPixel.cxx
						   lib/OledFont8x8.cxx
						   lib/OledFont8x12.cxx
						   lib/OledFont8x16.cxx
						   lib/OledGraphics.cxx
						   lib/OledI2C.cxx)

include_directories(${PROJECT_SOURCE_DIR}/lib)

set(EXTRA_LIBS ${EXTRA_LIBS} HD44780Lcd)

#--------------------------------------------------------------------------
find_package(InfluxDB REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)

add_executable(raspilogger raspilogger.cpp)
target_link_libraries(raspilogger PRIVATE SSD1306 InfluxData::InfluxDB Threads::Threads Boost::system)
