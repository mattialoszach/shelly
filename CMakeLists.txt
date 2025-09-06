cmake_minimum_required(VERSION 3.16)
project(shelly LANGUAGES CXX)

# ---- C++ / Build flags -------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Dependencies ------------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED IMPORTED_TARGET portaudio-2.0)
pkg_check_modules(SNDFILE   REQUIRED IMPORTED_TARGET sndfile)
find_package(CURL REQUIRED)

# ---- whisper.cpp as subproject (in-process STT) ------------------------------
# Turn off examples/tests from whisper.cpp
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
# (Optional) enable Metal explicitly (usually auto-detected)
# set(WHISPER_METAL ON CACHE BOOL "" FORCE)

add_definitions(-DGGML_METAL_NDEBUG)

add_subdirectory(third_party/whisper.cpp EXCLUDE_FROM_ALL) # creates target "whisper"

if(TARGET ggml-metal)
  target_compile_definitions(ggml-metal PRIVATE GGML_METAL_NDEBUG)
endif()

# ---- Executable --------------------------------------------------------------
add_executable(shelly
  src/main.cpp
  src/Recorder.cpp
  src/WavWriter.cpp
  src/WhisperOffline.cpp
  src/OllamaClient.cpp         # if you're using Ollama; otherwise remove
)

# ---- Includes ----------------------------------------------------------------
target_include_directories(shelly PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/third_party
  ${CMAKE_SOURCE_DIR}/third_party/whisper.cpp   # for whisper.h
)

# ---- Linker ------------------------------------------------------------------
target_link_libraries(shelly
  PkgConfig::PORTAUDIO
  PkgConfig::SNDFILE
  CURL::libcurl
  whisper
)

# macOS frameworks / acceleration
if(APPLE)
  target_link_libraries(shelly "-framework Accelerate")
  target_link_libraries(shelly
    "-framework CoreAudio" "-framework AudioToolbox" "-framework AudioUnit" "-framework CoreServices")
endif()