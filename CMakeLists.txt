cmake_minimum_required(VERSION 2.8.9)

project(skanlite)


find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include_directories (${KDE4_INCLUDES})

find_package(KSane REQUIRED)
find_package(PNG REQUIRED)

set(SKANLITE_VERSION_MAJOR "1")
set(SKANLITE_VERSION_MINOR "1")
set(SKANLITE_VERSION_STRING "${SKANLITE_VERSION_MAJOR}.${SKANLITE_VERSION_MINOR}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/version.h)


add_subdirectory(src)
add_subdirectory(doc)

