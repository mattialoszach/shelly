cmake_minimum_required(VERSION 3.16)
project(shelly LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
# Use IMPORTED_TARGET -> creates PkgConfig::PORTAUDIO & PkgConfig::SNDFILE
pkg_check_modules(PORTAUDIO REQUIRED IMPORTED_TARGET portaudio-2.0)
pkg_check_modules(SNDFILE   REQUIRED IMPORTED_TARGET sndfile)
find_package(CURL REQUIRED)

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/third_party
)

add_executable(shelly
  src/main.cpp
  src/Recorder.cpp
  # add all additional .cpp-fies if existing
)

target_link_libraries(shelly
  PkgConfig::PORTAUDIO
  PkgConfig::SNDFILE
  CURL::libcurl
)

if(APPLE)
  target_link_libraries(shelly
    "-framework CoreAudio" "-framework AudioToolbox" "-framework AudioUnit" "-framework CoreServices")
endif()