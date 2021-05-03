cmake_minimum_required(VERSION 3.12.0)
project(raspiloggeroled)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -mfpu=vfp -mfloat-abi=hard -mtune=arm1176jzf-s -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(JSON_LIB_URL "https://github.com/nlohmann/json/releases/latest/download/json.hpp")
set(JSON_DL_PATH "nlohmann_json/json.hpp")

if (NOT EXISTS JSON_DL_PATH)
    file(DOWNLOAD "${JSON_LIB_URL}" "${JSON_DL_PATH}")
endif()

#--------------------------------------------------------------------------

add_library(SSD1306 STATIC lib/FileDescriptor.cxx
						   lib/OledHardware.cxx
						   lib/OledPixel.cxx
						   lib/OledFont8x8.cxx
						   #lib/OledFont8x12.cxx
						   #lib/OledFont8x16.cxx
						   lib/OledGraphics.cxx
						   lib/OledI2C.cxx)

include_directories(${PROJECT_SOURCE_DIR}/lib)

include_directories(nlohmann_json)

#set(EXTRA_LIBS ${EXTRA_LIBS} HD44780Lcd)

#--------------------------------------------------------------------------
find_package(InfluxDB REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)

add_executable(raspilogger raspilogger.cpp)
add_executable(oledupdater oledupdater.cpp)

target_link_libraries(oledupdater PRIVATE SSD1306 InfluxData::InfluxDB Boost::system nlohmann_json::nlohmann_json)
target_link_libraries(raspilogger PRIVATE InfluxData::InfluxDB Boost::system rf24-bcm rf24network nlohmann_json::nlohmann_json)