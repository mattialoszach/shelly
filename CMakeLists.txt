cmake_minimum_required(VERSION 3.16)
project(shelly LANGUAGES CXX)

# ---- C++ / Build flags -------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SHELLY_SHARE_DIR "${CMAKE_INSTALL_PREFIX}/share/shelly" CACHE PATH "Shelly shared data dir")
add_compile_definitions(DEFAULT_SHELLY_SHARE_DIR="${SHELLY_SHARE_DIR}")

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
  src/TTS.cpp
  src/TextPrep.cpp
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
  target_link_libraries(shelly
    "-framework Accelerate"
    "-framework CoreAudio" "-framework AudioToolbox" "-framework AudioUnit" "-framework CoreServices"
  )
  # Enable modern rpath handling on macOS
  set(CMAKE_MACOSX_RPATH ON)
endif()

# Embed a relative install RPATH so the binary can find its private libs
set_target_properties(shelly PROPERTIES
  INSTALL_RPATH "$<IF:$<PLATFORM_ID:Darwin>,@loader_path/../libexec/shelly,$ORIGIN/../libexec/shelly>"
)

# ---- Install rules -----------------------------------------------------------
# Install binary under chosen prefix (local/user/system)
install(TARGETS shelly RUNTIME DESTINATION bin)

# Optionally install bundled whisper models if present in the source tree
if(EXISTS ${CMAKE_SOURCE_DIR}/models)
  install(DIRECTORY models/
          DESTINATION share/shelly/models
          FILES_MATCHING PATTERN "*.bin")
endif()

# Silence dev warnings about PUBLIC_HEADER destinations for third-party libs
if(TARGET whisper)
  set_target_properties(whisper PROPERTIES PUBLIC_HEADER "")
endif()
foreach(_t IN ITEMS ggml ggml-base ggml-cpu ggml-metal ggml-blas)
  if(TARGET ${_t})
    set_target_properties(${_t} PROPERTIES PUBLIC_HEADER "")
  endif()
endforeach()

# Also install the whisper library to the chosen prefix lib dir (shared or static)
install(TARGETS whisper
  ARCHIVE DESTINATION libexec/shelly
  LIBRARY DESTINATION libexec/shelly
  RUNTIME DESTINATION libexec/shelly
)

# Install ggml runtime libraries alongside whisper so dyld/ld can resolve them
if(TARGET ggml)
  install(TARGETS ggml
    ARCHIVE DESTINATION libexec/shelly
    LIBRARY DESTINATION libexec/shelly
    RUNTIME DESTINATION libexec/shelly)
endif()
if(TARGET ggml-cpu)
  install(TARGETS ggml-cpu
    ARCHIVE DESTINATION libexec/shelly
    LIBRARY DESTINATION libexec/shelly
    RUNTIME DESTINATION libexec/shelly)
endif()
if(TARGET ggml-metal)
  install(TARGETS ggml-metal
    ARCHIVE DESTINATION libexec/shelly
    LIBRARY DESTINATION libexec/shelly
    RUNTIME DESTINATION libexec/shelly)
endif()
if(TARGET ggml-base)
  install(TARGETS ggml-base
    ARCHIVE DESTINATION libexec/shelly
    LIBRARY DESTINATION libexec/shelly
    RUNTIME DESTINATION libexec/shelly)
endif()
if(TARGET ggml-blas)
  install(TARGETS ggml-blas
    ARCHIVE DESTINATION libexec/shelly
    LIBRARY DESTINATION libexec/shelly
    RUNTIME DESTINATION libexec/shelly)
endif()
