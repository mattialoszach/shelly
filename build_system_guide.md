# 🐚 Build-System Guide
## Remove previous build
rm -rf build

## New Build-directory, CMakeLists.txt changes or new packages installed
cmake -S . -B build

## Compile new C++ Code => New binary
cmake --build build -j

## Run the binary
./build/shelly