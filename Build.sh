#!/bin/bash
set -e

# Default to Debug if no argument is passed (Usage: ./Build.sh [Debug|Release])
BUILD_TYPE=${1:-Debug}
BUILD_DIR="Build_${BUILD_TYPE}"

echo "Building ProjectCam in ${BUILD_TYPE} mode..."
echo "Target build directory: ${BUILD_DIR}"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure CMake with the requested build type pointing to parent root
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..

# Compile using 2 parallel jobs to prevent OOM on Pi Zero 2 W (512MB RAM)
cmake --build . --parallel 2

echo "Build complete! Executable is located in ${BUILD_DIR}/ProjectCam."