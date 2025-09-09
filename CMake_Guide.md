## Shelly Build & Install Guide

This project uses CMake (>= 3.16). The top-level `CMakeLists.txt` builds the `shelly` binary and optionally installs Whisper model files into a standard share directory so the app can run from anywhere.

### Prerequisites

- CMake 3.16+
- C/C++ toolchain (clang/clang++)
- Libraries:
  - PortAudio (`portaudio` / `portaudio-2.0`)
  - libsndfile (`sndfile`)
  - libcurl (`curl` / `libcurl`)

Examples:
- macOS (Homebrew): `brew install portaudio libsndfile curl`
- Ubuntu/Debian: `sudo apt install cmake build-essential libportaudio2 portaudio19-dev libsndfile1-dev libcurl4-openssl-dev`

### Configure and Build

```bash
# Configure (Release build recommended)
cmake -S . -B build

# Build
cmake --build build -j

# Run from build folder (no install)
./build/shelly
```

Tip: If you need a clean build, remove the `build/` folder first: `rm -rf build`.

### Install Options (choose one)

You can install locally (into a test folder), into your user prefix, or system-wide. The install layout is:
- Binary: `<prefix>/bin/shelly`
- Models: `<prefix>/share/shelly/models/*.bin`

1) Local (inside the repository, good for testing):
```bash
cmake --install build --prefix build/local
# Binary:
build/local/bin/shelly
# Models (if present in ./models during install):
build/local/share/shelly/models/
```

2) User prefix (no root required):
```bash
cmake --install build --prefix ~/.local
export PATH="$HOME/.local/bin:$PATH"   # put in your shell profile
```

3) **System-wide (requires sudo)**:
```bash
sudo cmake --install build --prefix /usr/local
# Binary:  /usr/local/bin/shelly
# Models:  /usr/local/share/shelly/models/
```

### Model Placement and Runtime Lookup

At runtime, Shelly looks for the Whisper model in this order:
1. Installed share directory: `<prefix>/share/shelly/models/ggml-base.en.bin`
2. Fallback: `./models/ggml-base.en.bin` (relative to current working directory)

Notes:
- If you keep a `models/` folder in the project root during `cmake --install`, all `*.bin` models are copied to the install share directory.
- You can also manually place your model file in the appropriate share directory after installation.

### Common Tasks

- Reconfigure after changing `CMakeLists.txt` or adding packages:
  ```bash
  cmake -S . -B build
  ```
- Rebuild after changing code:
  ```bash
  cmake --build build -j
  ```
- Install to test location and run:
  ```bash
  cmake --install build --prefix build/local
  build/local/bin/shelly
  ```

### macOS Notes

- Accelerate and CoreAudio frameworks are linked automatically.
- TTS uses the system `say` command by default.

### Troubleshooting

- “install TARGETS given target … which does not exist”: Ensure `install(TARGETS shelly …)` appears after `add_executable(shelly …)` in `CMakeLists.txt`.
- Missing libs: Install PortAudio, libsndfile, and libcurl development packages.
- Model not found: Make sure `ggml-base.en.bin` is in either `<prefix>/share/shelly/models/` or `./models/` when running.