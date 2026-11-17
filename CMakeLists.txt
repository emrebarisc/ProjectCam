cmake_minimum_required(VERSION 3.20)

set(PROGRAM_NAME ProjectCam)

project(${PROGRAM_NAME})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Debug if no build type is specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(PROJECT_DIR "${CMAKE_CURRENT_LIST_DIR}/")
set(SOURCE_DIR_NAME "Source")
set(INCLUDE_DIR_NAME "Include")

set(SUBDIRS
    ${SOURCE_DIR_NAME}
    ${SOURCE_DIR_NAME}/IO
    ${SOURCE_DIR_NAME}/Managers)

set(SOURCE_FILES)
foreach(SUBDIR ${SUBDIRS})
    set(COMBINED_DIR ${PROJECT_DIR}${SUBDIR})
    file(GLOB GROUP_FILES "${COMBINED_DIR}/*")
    source_group(${SUBDIR} FILES ${GROUP_FILES})
    list(APPEND SOURCE_FILES ${GROUP_FILES})
endforeach()

# Optimized flags for Release mode (Pi Zero 2 W / Cortex-A53)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(RPI_FLAGS "-march=native -Ofast -ftree-vectorize -pipe -fomit-frame-pointer -funroll-loops")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RPI_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${RPI_FLAGS}")
endif()

add_executable(${PROGRAM_NAME} ${SOURCE_FILES})

target_include_directories(${PROGRAM_NAME} PUBLIC
    /usr/include/libcamera
    ${PROJECT_DIR}/${SOURCE_DIR_NAME}
    ${PROJECT_DIR}/${INCLUDE_DIR_NAME}
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)

target_link_libraries(${PROGRAM_NAME} PUBLIC
    camera
    camera-base
    PkgConfig::GPIOD
)